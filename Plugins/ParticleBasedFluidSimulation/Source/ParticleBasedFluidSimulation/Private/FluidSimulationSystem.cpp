#include "FluidSimulationSystem.h"

FFluidSimulationSystem::FFluidSimulationSystem() {

}

void FFluidSimulationSystem::InitializeParticles(TArray<FParticle>& InParticles) {
    Particles = InParticles;
}

void FFluidSimulationSystem::StepSimulation(float DeltaTime) {
    
}