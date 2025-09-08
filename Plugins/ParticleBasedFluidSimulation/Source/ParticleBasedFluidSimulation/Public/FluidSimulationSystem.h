#pragma once

#include "CoreMinimal.h"
#include "FluidParticle.h"

class FFluidSimulationSystem
{
public:
    FFluidSimulationSystem();
    void InitializeParticles(TArray<FParticle>& InParticles);
    void StepSimulation(float DeltaTime);

    const TArray<FParticle>& GetParticles() const { return Particles; }
private:
    TArray<FParticle> Particles;

    void ComputeForceDensityField() {}
    void ComputePressureForces() {}
    void SmoothingKernel(const FVector& Position) {}
    void Integrate(float DeltaTime) {}
};