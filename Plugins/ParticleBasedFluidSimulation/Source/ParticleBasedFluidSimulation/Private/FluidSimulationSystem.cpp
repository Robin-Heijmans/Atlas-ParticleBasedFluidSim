#include "FluidSimulationSystem.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

FFluidSimulationSystem::FFluidSimulationSystem() {

}

void FFluidSimulationSystem::InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB) {
    Particles = InParticles;
    TableSize = Particles.Num();
    SpatialLookup.SetNum(TableSize);
    StartIndices.SetNum(TableSize);
    ExternalForces.Init(FVector::ZeroVector, TableSize);

    MinBounds = MinB;
    MaxBounds = MaxB;

    UpdateSpatialLookup(SmoothingRadius);
}

void FFluidSimulationSystem::SetVolumeBounds(FVector& MinB, FVector& MaxB) {
    MinBounds = MinB;
    MaxBounds = MaxB;
}

void FFluidSimulationSystem::StepSimulation(float DeltaTime) {
    if (Particles.IsEmpty()) return;
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Particles exist in system"));
    const float LookAheadTimeStep = 1.f / 120.f;
    for (int i = 0; i < Particles.Num(); i++) {
        Particles[i].Velocity += (ExternalForces[i] + Gravity) * DeltaTime;
        Particles[i].PredictedPosition = Particles[i].Position + Particles[i].Velocity * LookAheadTimeStep;
    }
    // Reset external forces
    ExternalForces.Init(FVector::ZeroVector, TableSize);
    UpdateSpatialLookup(SmoothingRadius);

    for (int i = 0; i < Particles.Num(); i++) {
        Particles[i].Density = CalculateDensity(Particles[i].PredictedPosition, i);
    }
    GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Green, (FString::Printf(TEXT("Particle[0] Density: %f"), Particles[0].Density)));

    for (int i = 0; i < Particles.Num(); i++) {
        FVector PressureForce = CalculatePressureForce(Particles[i].PredictedPosition, i);
        FVector PressureAcceleration = -PressureForce / Particles[i].Density;
        Particles[i].Velocity += PressureAcceleration * DeltaTime;
    }

    for (int i = 0; i < Particles.Num(); i++) {
        FVector ViscosityForce = CalculateViscosityForce(Particles[i].PredictedPosition, i);
        Particles[i].Velocity += ViscosityForce * DeltaTime;
    }

    for (auto& particle : Particles) {
        particle.Position += particle.Velocity * DeltaTime;
        ResolveCollisions(particle);
    }
}

void FFluidSimulationSystem::ApplySettings(FFluidSimSettings& settings) {
    Gravity = settings.Gravity;
    PressureAmplifier = settings.PressureAmplifier;
    TargetDensity = settings.TargetDensity;
    CollisionDampening = settings.CollisionDampening;
    SmoothingRadius = settings.SmoothingRadius;
    ViscosityStrength = settings.ViscosityStrength;
}

void FFluidSimulationSystem::ResolveCollisions(FParticle& particle) {
    if (particle.Position.X <= MinBounds.X || particle.Position.X >= MaxBounds.X) {
        particle.Position.X = FMath::Clamp(particle.Position.X, MinBounds.X, MaxBounds.X);
        particle.Velocity.X *= -1.f * CollisionDampening;
    }
    if (particle.Position.Y <= MinBounds.Y || particle.Position.Y >= MaxBounds.Y) {
        particle.Position.Y = FMath::Clamp(particle.Position.Y, MinBounds.Y, MaxBounds.Y);
        particle.Velocity.Y *= -1.f * CollisionDampening;
    }
    if (particle.Position.Z <= MinBounds.Z || particle.Position.Z >= MaxBounds.Z) {
        particle.Position.Z = FMath::Clamp(particle.Position.Z, MinBounds.Z, MaxBounds.Z);
        particle.Velocity.Z *= -1.f * CollisionDampening;
    }
}

void FFluidSimulationSystem::UpdateSpatialLookup(const float& Radius) {
    for (int i = 0; i < Particles.Num(); i++) {
        FIntVector CellCoords = PositionToCellCoords(Particles[i].PredictedPosition, Radius);
        uint32 CellKey = GetKeyFromHash(HashCell(CellCoords));
        SpatialLookup[i] = {CellKey, i};
        StartIndices[i] = -1;
    }
    SpatialLookup.Sort();

    for (int i = 0; i < Particles.Num(); i++) {
        uint32 Key = SpatialLookup[i].Key;
        uint32 KeyPrevious = i == 0 ? -1 : SpatialLookup[i-1].Key;
        if (Key != KeyPrevious) {
            StartIndices[Key] = i;
        }
    }
}

float FFluidSimulationSystem::ConvertDensityToPressure(const float& Density) {
    float DensityError = Density - TargetDensity;
    return DensityError * PressureAmplifier;
}

float FFluidSimulationSystem::CalculateSharedPressure(const float& DensityA, const float& DensityB) {
    float PressureA = ConvertDensityToPressure(DensityA);
    float PressureB = ConvertDensityToPressure(DensityB);
    return (PressureA + PressureB) *0.5f;
}

float FFluidSimulationSystem::SmoothingKernel(const float& Distance, const float& Radius) {
    float Volume =  15 / (2 * PI * FMath::Pow(Radius, 5));
    float Value = Radius-Distance;
    return Value * Value * Volume;
}

float FFluidSimulationSystem::SmoothingKernelDerivative(const float& Distance, const float& Radius) {
    float Volume = 15 / (FMath::Pow(Radius, 5) * PI);
    float Value = Radius - Distance;
    return -Value * Volume;
}

float FFluidSimulationSystem::SmoothingKernelViscosity(const float& Distance, const float& Radius) {
    float Volume = 315 / (64 * PI * FMath::Pow(Radius, 9));
    float Value = Radius * Radius - Distance * Distance;
    return Value * Value * Value * Volume;
}

float FFluidSimulationSystem::CalculateDensity(const FVector& Position, const int Index) {
    float Density = 0.0f;

    FIntVector CentreCoords = PositionToCellCoords(Position, SmoothingRadius);
    for (const auto& cellOffset : Offsets3D) {
        uint32 Key = GetKeyFromHash(HashCell(CentreCoords + cellOffset));
        uint32 StartIndex = StartIndices[Key];
        for (uint32 i = StartIndex; i < TableSize; i++) {
            if (SpatialLookup[i].Key != Key) break;
            int ParticleIndex = SpatialLookup[i].ParticleIndex;
            //if (ParticleIndex == Index) continue;
            FVector Offset = Particles[ParticleIndex].PredictedPosition - Position;
            float SqrDistance = FVector::DotProduct(Offset, Offset);
            if (SqrDistance <= (SmoothingRadius * SmoothingRadius)) {
                float Distance = FMath::Sqrt(SqrDistance);
                float Influence = SmoothingKernel(Distance, SmoothingRadius);
                Density += Influence * Particles[ParticleIndex].Mass;
            }
        }
    }
    return Density;
}

FVector FFluidSimulationSystem::CalculatePressureForce(const FVector& Position, const int Index) {
    FVector PressureForce = FVector::ZeroVector; 
    FIntVector CentreCoords = PositionToCellCoords(Position, SmoothingRadius);
    for (const auto& cellOffset : Offsets3D) {
        uint32 Key = GetKeyFromHash(HashCell(CentreCoords + cellOffset));
        uint32 StartIndex = StartIndices[Key];
        for (uint32 i = StartIndex; i < TableSize; i++) {
            if (SpatialLookup[i].Key != Key) break;
            int ParticleIndex = SpatialLookup[i].ParticleIndex;
            if (ParticleIndex == Index) continue;
            FVector Offset = Particles[ParticleIndex].PredictedPosition - Position;
            float SqrDistance = FVector::DotProduct(Offset, Offset);
            if (SqrDistance <= (SmoothingRadius * SmoothingRadius)) {
                float Distance = FMath::Sqrt(SqrDistance);
                FVector Direction = Distance == 0 ? FVector::UpVector : Offset / Distance;
                float Slope = SmoothingKernelDerivative(Distance, SmoothingRadius);
                float Density = Particles[ParticleIndex].Density;
                float SharedPressure = CalculateSharedPressure(Density, Particles[Index].Density);
                PressureForce += SharedPressure * Direction * Slope * Particles[ParticleIndex].Mass / FMath::Min(Density, Sven);
            }
        }
    }

    return PressureForce;
}

FVector FFluidSimulationSystem::CalculateViscosityForce(const FVector& Position, const int Index) {
    FVector ViscosityForce = FVector::ZeroVector; 
    FIntVector CentreCoords = PositionToCellCoords(Position, SmoothingRadius);
    for (const auto& cellOffset : Offsets3D) {
        uint32 Key = GetKeyFromHash(HashCell(CentreCoords + cellOffset));
        uint32 StartIndex = StartIndices[Key];
        for (uint32 i = StartIndex; i < TableSize; i++) {
            if (SpatialLookup[i].Key != Key) break;
            int ParticleIndex = SpatialLookup[i].ParticleIndex;
            if (ParticleIndex == Index) continue;
            FVector Offset = Particles[ParticleIndex].PredictedPosition - Position;
            float SqrDistance = FVector::DotProduct(Offset, Offset);
            if (SqrDistance <= (SmoothingRadius * SmoothingRadius)) {
                float Distance = FMath::Sqrt(SqrDistance);
                float Influence = SmoothingKernelViscosity(Distance, SmoothingRadius);
                ViscosityForce += (Particles[ParticleIndex].Velocity - Particles[Index].Velocity) * Influence;
            }
        }
    }
    return ViscosityForce * ViscosityStrength;
}

FIntVector FFluidSimulationSystem::PositionToCellCoords(const FVector& Position, const float& Radius) {
    uint32 CellX = Position.X/Radius;
    uint32 CellY = Position.Y/Radius;
    uint32 CellZ = Position.Z/Radius;
    return FIntVector(CellX, CellY, CellZ);
}

FVector FFluidSimulationSystem::CellToPosition(const FIntVector& Cell, const float& Radius) {
    return FVector(Cell.X, Cell.Y, Cell.Z) * Radius;
}

uint32 FFluidSimulationSystem::HashCell(const FIntVector& CellCoords) {
    return CellCoords.X * HashKey1 + CellCoords.Y * HashKey2 + CellCoords.Z * HashKey3; // Multiply with 3 prime numbers
}

uint32 FFluidSimulationSystem::GetKeyFromHash(const uint32& Hash) {
    return Hash % TableSize;
}

void FFluidSimulationSystem::ApplyExternalForce(const FVector& Location, const float& ForceAmplifier, const float& radius) {
    FIntVector CentreCoords = PositionToCellCoords(Location, SmoothingRadius);
    
    for (const auto& cellOffset : Offsets3D) {
        uint32 Key = GetKeyFromHash(HashCell(CentreCoords + cellOffset));
        uint32 StartIndex = StartIndices[Key];
        for (uint32 i = StartIndex; i < TableSize; i++) {
            if (SpatialLookup[i].Key != Key) break;
            int ParticleIndex = SpatialLookup[i].ParticleIndex;
            FVector Offset = Particles[ParticleIndex].PredictedPosition - Location;
            float SqrDistance = FVector::DotProduct(Offset, Offset);
            if (SqrDistance <= (radius * radius)) {
                float Distance = FMath::Sqrt(SqrDistance);
                FVector Direction = Distance == 0 ? FVector::UpVector : Offset / Distance;
                ExternalForces[ParticleIndex] += ForceAmplifier * Direction;
            }
        }
    }
}

void FFluidSimulationSystem::BoxCollision(const UBoxComponent& OtherComp, const UBoxComponent& Bounds, const FVector& LocalPos) {
    // Transform other to local
    FVector WorldScaleBounds = Bounds.GetComponentScale();
    FVector TranslatedCenter = OtherComp.GetComponentLocation() - Bounds.GetComponentLocation();
    FVector LocalCenter = TranslatedCenter / WorldScaleBounds;
    FVector LocalScale = OtherComp.GetComponentScale() / WorldScaleBounds;

    FVector ObjectExtent = OtherComp.GetUnscaledBoxExtent() * LocalScale;
    FVector ObjMinBounds = LocalCenter - ObjectExtent;
    FVector ObjMaxBounds = LocalCenter + ObjectExtent;

    FVector ThisExtent = Bounds.GetUnscaledBoxExtent();
    FVector LocalMin = -ThisExtent;
	FVector LocalMax = ThisExtent;

    FVector IntersectionMin = FVector(FMath::Max(ObjMinBounds.X, LocalMin.X),
                                      FMath::Max(ObjMinBounds.Y, LocalMin.Y),
                                      FMath::Max(ObjMinBounds.Z, LocalMin.Z));
    
    FVector IntersectionMax = FVector(FMath::Min(ObjMaxBounds.X, LocalMax.X),
                                      FMath::Min(ObjMaxBounds.Y, LocalMax.Y),
                                      FMath::Min(ObjMaxBounds.Z, LocalMax.Z));
    FIntVector MinCoords = PositionToCellCoords(IntersectionMin, SmoothingRadius);
    FIntVector MaxCoords = PositionToCellCoords(IntersectionMax, SmoothingRadius);

    for (int CellX = MinCoords.X; CellX <= MaxCoords.X; CellX++) {
        for (int CellY = MinCoords.Y; CellY <= MaxCoords.Y; CellY++) {
            for (int CellZ = MinCoords.Z; CellZ <= MaxCoords.Z; CellZ++) {
                FIntVector CellCoords = FIntVector(CellX, CellY, CellZ);
                uint32 Key = GetKeyFromHash(HashCell(CellCoords));
                uint32 StartIndex = StartIndices[Key];
                for (uint32 i = StartIndex; i < TableSize; i++) {
                    if (SpatialLookup[i].Key != Key) break;
                    int ParticleIndex = SpatialLookup[i].ParticleIndex;
                    FVector& Pos = Particles[ParticleIndex].Position;
                    if (CheckBoxCollision(Pos, IntersectionMin, IntersectionMax)) {
                        GEngine->AddOnScreenDebugMessage(4, 1.f, FColor::Yellow, (FString::Printf(TEXT("Box collision detected"))));
                        FVector& Vel = Particles[ParticleIndex].Velocity;
                        FVector DistToMin = Pos - ObjMinBounds;
                        FVector DistToMax = ObjMaxBounds - Pos;

                        float MinPenX = FMath::Min(FMath::Abs(DistToMin.X), FMath::Abs(DistToMax.X));
                        float MinPenY = FMath::Min(FMath::Abs(DistToMin.Y), FMath::Abs(DistToMax.Y));
                        float MinPenZ = FMath::Min(FMath::Abs(DistToMin.Z), FMath::Abs(DistToMax.Z));

                        float SmallestPen = MinPenX;
                        FVector PushDir = FVector::ZeroVector;

                        if (MinPenY < SmallestPen) { SmallestPen = MinPenY; }
                        if (MinPenZ < SmallestPen) { SmallestPen = MinPenZ; }

                        if (SmallestPen == MinPenX) {
                            if (FMath::Abs(DistToMin.X) < FMath::Abs(DistToMax.X)) {
                                PushDir = FVector::BackwardVector;
                            }
                            else {
                                PushDir = FVector::ForwardVector;
                            }
                        } else if (SmallestPen == MinPenY) {
                            if (FMath::Abs(DistToMin.Y) < FMath::Abs(DistToMax.Y)) {
                                PushDir = FVector::LeftVector;
                            }
                            else {
                                PushDir = FVector::RightVector;
                            }
                        } else {
                            if (FMath::Abs(DistToMin.Z) < FMath::Abs(DistToMax.Z)) {
                                PushDir = FVector::DownVector;
                            }
                            else {
                                PushDir = FVector::UpVector;
                            }
                        }
                        Pos += PushDir * SmallestPen;
                        Vel *= (-PushDir.GetAbs() * CollisionDampening + (FVector::OneVector - PushDir.GetAbs()));
                    }
                }
            }
        }
    }
}

void FFluidSimulationSystem::SphereCollision(const USphereComponent& OtherComp, const UBoxComponent& Bounds, const FVector& LocalPos, UWorld* World) {
    FVector WorldScaleBounds = Bounds.GetComponentScale();
    FVector TranslatedCenter = OtherComp.GetComponentLocation() - Bounds.GetComponentLocation();
    FVector LocalCenter = TranslatedCenter / WorldScaleBounds;
    FVector LocalScale = OtherComp.GetComponentScale() / WorldScaleBounds;

    FVector SphereRadius3D = OtherComp.GetUnscaledSphereRadius() * LocalScale;
    FVector ObjMinBounds = LocalCenter - SphereRadius3D;
    FVector ObjMaxBounds = LocalCenter + SphereRadius3D;

    FVector ThisExtent = Bounds.GetUnscaledBoxExtent();
    FVector LocalMin = -ThisExtent;
	FVector LocalMax = ThisExtent;

    FVector IntersectionMin = FVector(FMath::Max(ObjMinBounds.X, LocalMin.X),
                                      FMath::Max(ObjMinBounds.Y, LocalMin.Y),
                                      FMath::Max(ObjMinBounds.Z, LocalMin.Z));
    
    FVector IntersectionMax = FVector(FMath::Min(ObjMaxBounds.X, LocalMax.X),
                                      FMath::Min(ObjMaxBounds.Y, LocalMax.Y),
                                      FMath::Min(ObjMaxBounds.Z, LocalMax.Z));
    FVector SafetyMargin = FVector(0.5f * SmoothingRadius);
    FIntVector MinCoords = PositionToCellCoords(IntersectionMin - SafetyMargin, SmoothingRadius);
    FIntVector MaxCoords = PositionToCellCoords(IntersectionMax + SafetyMargin, SmoothingRadius);
    
    for (int CellX = MinCoords.X; CellX <= MaxCoords.X; CellX++) {
        for (int CellY = MinCoords.Y; CellY <= MaxCoords.Y; CellY++) {
            for (int CellZ = MinCoords.Z; CellZ <= MaxCoords.Z; CellZ++) {
                FIntVector CellCoords = FIntVector(CellX, CellY, CellZ);
                GEngine->AddOnScreenDebugMessage(4, 1.f, FColor::Yellow, (FString::Printf(TEXT("Sphere collision detected"))));
                //if (!CheckSphereCellCollision(CellCoords, LocalCenter, SphereRadius3D)) continue;

                uint32 Key = GetKeyFromHash(HashCell(CellCoords));
                uint32 StartIndex = StartIndices[Key];
                for (uint32 i = StartIndex; i < TableSize; i++) {
                    if (SpatialLookup[i].Key != Key) break;
                    int ParticleIndex = SpatialLookup[i].ParticleIndex;
                    FVector& Pos = Particles[ParticleIndex].Position;
                    FVector Offset = (Pos - LocalCenter) / SphereRadius3D;
                    if (CheckSphereCollision(Offset)) {
                        FVector& Vel = Particles[ParticleIndex].Velocity;
                        FVector Direction = Offset / Offset.Size();
                        FVector OnSurfaceWorld = ((1.f - Offset.Size()) * SphereRadius3D) * Direction;
                        Pos += OnSurfaceWorld;
                        Vel = ReflectVelocity(Vel, Direction) * CollisionDampening;
                    }
                }
            }
        }
    }
}

void FFluidSimulationSystem::CapsuleCollision(const UCapsuleComponent& OtherComp, const UBoxComponent& Bounds, const FVector& LocalPos) {
    
}

bool FFluidSimulationSystem::CheckBoxCollision(const FVector& Position, const FVector& IntersectionMinBounds, const FVector& IntersectionMaxBounds) {
    if (Position.X < IntersectionMinBounds.X || Position.X > IntersectionMaxBounds.X
    || Position.Y < IntersectionMinBounds.Y || Position.Y > IntersectionMaxBounds.Y
    || Position.Z < IntersectionMinBounds.Z || Position.Z > IntersectionMaxBounds.Z) {
        return false;
    }
    else return true;
}

bool FFluidSimulationSystem::CheckSphereCollision(const FVector& Position) {
    return Position.SizeSquared() <= 1.f;
}

bool FFluidSimulationSystem::CheckSphereCellCollision(const FIntVector& CellCoords, const FVector& SphereCenter, const FVector& Radius3D) {
    FVector CellCenter = CellToPosition(CellCoords, SmoothingRadius);
    FVector CellMin = CellCenter - SmoothingRadius;
    FVector CellMax = CellCenter + SmoothingRadius;
    FVector PosToCheck = FVector((CellMin.X < SphereCenter.X ? CellMin.X : CellMax.X, 
                               CellMin.Y < SphereCenter.Y ? CellMin.Y : CellMax.Y, 
                               CellMin.Z < SphereCenter.Z ? CellMin.Z : CellMax.Z));
    return CheckSphereCollision((PosToCheck - SphereCenter) / Radius3D);
}

FVector FFluidSimulationSystem::ReflectVelocity(const FVector& Vel, const FVector& Normal) {
    return Vel - 2.f * FVector::DotProduct(Vel, Normal) * Normal;
}