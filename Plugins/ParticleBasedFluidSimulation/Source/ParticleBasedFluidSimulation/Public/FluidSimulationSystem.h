#pragma once

#include "CoreMinimal.h"
#include "FluidParticle.h"

#include "FluidSimulationSystem.generated.h"

USTRUCT(BlueprintType)
struct FFluidSimSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    FVector Gravity = FVector(0.0f, 0.0f, -9.81f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float ForceAmplifier = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float CollisionDampening = 0.7f;
};

class FFluidSimulationSystem
{
public:
    FFluidSimulationSystem();
    void InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB);
    void StepSimulation(float DeltaTime);
    void ApplySettings(FFluidSimSettings& settings);

    const TArray<FParticle>& GetParticles() const { return *Particles; }
private:
    void ResolveCollisions(FParticle& particle);
    void ComputeForceDensityField();
    void ComputePressureForces();
    void SmoothingKernel(const FVector& Position);
    void Integrate(float DeltaTime);

    TArray<FParticle>* Particles;
    FVector MinBounds = FVector::ZeroVector;
    FVector MaxBounds = FVector::ZeroVector;

    // Simulation Settings
    FVector Gravity = FVector(0.0f, 0.0f, -9.81f);
    float ForceAmplifier = 5.0f;
    float CollisionDampening = 0.7f;
};