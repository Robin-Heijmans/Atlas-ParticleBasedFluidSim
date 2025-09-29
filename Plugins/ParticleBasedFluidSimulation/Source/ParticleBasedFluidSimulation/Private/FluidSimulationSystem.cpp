#include "FluidSimulationSystem.h"

FFluidSimulationSystem::FFluidSimulationSystem() {

}

void FFluidSimulationSystem::InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB) {
    Particles = InParticles;
    TableSize = Particles.Num();
    SpatialLookup.SetNum(TableSize);
    StartIndices.SetNum(TableSize);

    MinBounds = MinB;
    MaxBounds = MaxB;
}

void FFluidSimulationSystem::SetVolumeBounds(FVector& MinB, FVector& MaxB) {
    MinBounds = MinB;
    MaxBounds = MaxB;
}

void FFluidSimulationSystem::StepSimulation(float DeltaTime) {
    if (Particles.IsEmpty()) return;
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Particles exist in system"));
    const float LookAheadTimeStep = 1.f / 120.f;
    for (auto& particle : Particles) {
        particle.Velocity += Gravity * DeltaTime;
        particle.PredictedPosition = particle.Position + particle.Velocity * LookAheadTimeStep;
    }

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
    GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, (FString::Printf(TEXT("Particle[0] Velocity after pressure forces: X=%f Y=%f Z=%f"), Particles[0].Velocity.X, Particles[0].Velocity.Y, Particles[0].Velocity.Z)));

    for (int i = 0; i < Particles.Num(); i++) {
        FVector ViscosityForce = CalculateViscosityForce(Particles[i].PredictedPosition, i);
        Particles[i].Velocity += ViscosityForce * DeltaTime;
    }
    GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Blue, (FString::Printf(TEXT("Particle[0] Velocity after viscosity forces: X=%f Y=%f Z=%f"), Particles[0].Velocity.X, Particles[0].Velocity.Y, Particles[0].Velocity.Z)));

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
            if (SqrDistance < (SmoothingRadius * SmoothingRadius)) {
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
            if (SqrDistance < (SmoothingRadius * SmoothingRadius)) {
                float Distance = FMath::Sqrt(SqrDistance);
                FVector Direction = Distance == 0 ? FVector::UpVector : Offset / Distance;
                float Slope = SmoothingKernelDerivative(Distance, SmoothingRadius);
                float Density = FMath::Min(Particles[ParticleIndex].Density, 0.1f);
                float SharedPressure = CalculateSharedPressure(Density, Particles[Index].Density);
                PressureForce += SharedPressure * Direction * Slope * Particles[ParticleIndex].Mass / Density;
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
            if (SqrDistance < (SmoothingRadius * SmoothingRadius)) {
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

uint32 FFluidSimulationSystem::HashCell(const FIntVector& CellCoords) {
    return CellCoords.X * HashKey1 + CellCoords.Y * HashKey2 + CellCoords.Z * HashKey3; // Multiply with 3 prime numbers
}

uint32 FFluidSimulationSystem::GetKeyFromHash(const uint32& Hash) {
    return Hash % TableSize;
}