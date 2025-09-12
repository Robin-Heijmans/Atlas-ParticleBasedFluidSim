#include "FluidSimulationSystem.h"

FFluidSimulationSystem::FFluidSimulationSystem() {

}

void FFluidSimulationSystem::InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB) {
    Particles = InParticles;
    MinBounds = MinB;
    MaxBounds = MaxB;
}

void FFluidSimulationSystem::StepSimulation(float DeltaTime) {
    if (Particles.IsEmpty()) return;
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Particles exist in system"));
    const float LookAheadTimeStep = 1.f / 120.f;
    bool bFirst = true;
    for (auto& particle : Particles) {
        particle.Velocity += Gravity * DeltaTime;
        particle.PredictedPosition = particle.Position + particle.Velocity * LookAheadTimeStep;
        // Update densities
        particle.Density = CalculateDensity(particle.PredictedPosition);

        if (bFirst) {
            FString Msg = FString::Printf(TEXT("First particle density: %f"), particle.Density);
            //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
            bFirst = false;
        }
    }

    for (int i = 0; i < Particles.Num(); i++) {
        FVector PressureForce = CalculatePressureForce(Particles[i].PredictedPosition, i);
        FVector PressureAcceleration = PressureForce / Particles[i].Density;
        Particles[i].Velocity += PressureAcceleration * DeltaTime;
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
    return Value * Volume;
}

float FFluidSimulationSystem::CalculateDensity(const FVector& Position) {
    float Density = 0.0f;

    for (const auto& particle : Particles) {
        FVector Offset = particle.Position - Position;
        float SqrDistance = FVector::DotProduct(Offset, Offset);
        if (SqrDistance < (SmoothingRadius * SmoothingRadius)) {
            float Distance = FMath::Sqrt(SqrDistance);
            float Influence = SmoothingKernel(Distance, SmoothingRadius);
            Density += Influence * particle.Mass;
        }
    }
    return Density;
}

FVector FFluidSimulationSystem::CalculatePressureForce(const FVector& Position, const int Index) {
    FVector PressureForce = FVector::ZeroVector; 
    for (int i = 0; i < Particles.Num(); i++) {
        if (Index == i) continue;
        FVector Offset = Particles[i].Position - Position;
        float SqrDistance = FVector::DotProduct(Offset, Offset);
        if (SqrDistance < (SmoothingRadius * SmoothingRadius)) {
            float Distance = FMath::Sqrt(SqrDistance);
            FVector Direction = Distance == 0 ? FMath::VRand() : Offset / Distance;
            float Slope = SmoothingKernelDerivative(Distance, SmoothingRadius);
            float Density = Particles[i].Density;
            float SharedPressure = CalculateSharedPressure(Density, Particles[Index].Density);
            PressureForce += SharedPressure * Direction * Slope * Particles[i].Mass / Density;
        }
    }
    return PressureForce;
}