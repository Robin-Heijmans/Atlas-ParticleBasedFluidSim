#include "FluidSimulationSystem.h"

FFluidSimulationSystem::FFluidSimulationSystem() {

}

void FFluidSimulationSystem::InitializeParticles(TArray<FParticle>& InParticles) {
    Particles = &InParticles;
}

void FFluidSimulationSystem::StepSimulation(float DeltaTime) {
    if (!Particles) return;
    GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Particles exist in system"));
    float dt2 = DeltaTime * DeltaTime;
    for (auto& particle : *Particles) {
        particle.Acceleration = Gravity;
        particle.Position += particle.Acceleration * dt2;
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