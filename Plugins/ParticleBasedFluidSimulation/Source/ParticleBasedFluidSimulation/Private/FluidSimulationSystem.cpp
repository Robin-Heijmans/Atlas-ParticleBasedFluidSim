#include "FluidSimulationSystem.h"

FFluidSimulationSystem::FFluidSimulationSystem() {

}

void FFluidSimulationSystem::InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB) {
    Particles = &InParticles;
    MinBounds = MinB;
    MaxBounds = MaxB;
}

void FFluidSimulationSystem::StepSimulation(float DeltaTime) {
    if (!Particles) return;
    // GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Particles exist in system"));
    for (auto& particle : *Particles) {
        particle.Velocity += Gravity * ForceAmplifier * DeltaTime;
        particle.Position += particle.Velocity * DeltaTime;
        ResolveCollisions(particle);
    }
}

void FFluidSimulationSystem::ApplySettings(FFluidSimSettings& settings) {
    Gravity = settings.Gravity;
    ForceAmplifier = settings.ForceAmplifier;
    CollisionDampening = settings.CollisionDampening;
}

void FFluidSimulationSystem::ResolveCollisions(FParticle& particle) {
    if (particle.Position.X <= MinBounds.X || particle.Position.X >= MaxBounds.X)
    {
        particle.Position.X = FMath::Clamp(particle.Position.X, MinBounds.X, MaxBounds.X);
        particle.Velocity.X *= -1.f * CollisionDampening;
    }
    if (particle.Position.Y <= MinBounds.Y || particle.Position.Y >= MaxBounds.Y)
    {
        particle.Position.Y = FMath::Clamp(particle.Position.Y, MinBounds.Y, MaxBounds.Y);
        particle.Velocity.Y *= -1.f * CollisionDampening;
    }
    if (particle.Position.Z <= MinBounds.Z || particle.Position.Z >= MaxBounds.Z)
    {
        particle.Position.Z = FMath::Clamp(particle.Position.Z, MinBounds.Z, MaxBounds.Z);
        particle.Velocity.Z *= -1.f * CollisionDampening;
    }
}

void FFluidSimulationSystem::ComputeForceDensityField() {

}

void FFluidSimulationSystem::ComputePressureForces() {

}

void FFluidSimulationSystem::SmoothingKernel(const FVector& Position) {

}

void FFluidSimulationSystem::Integrate(float DeltaTime) {
    
}