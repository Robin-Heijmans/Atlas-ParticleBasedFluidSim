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
    float PressureAmplifier = 10.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float TargetDensity = 2.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float CollisionDampening = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float SmoothingRadius = 4.f;
};

class FFluidSimulationSystem
{
public:
    FFluidSimulationSystem();
    void InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB);
    void StepSimulation(float DeltaTime);
    void ApplySettings(FFluidSimSettings& settings);

    const TArray<FParticle>& GetParticles() const { return Particles; }
private:
    void ResolveCollisions(FParticle& particle);
    float ConvertDensityToPressure(const float& Density);
    float CalculateSharedPressure(const float& DensityA, const float& DensityB);
    float SmoothingKernel(const float& Distance, const float& Radius);
    float SmoothingKernelDerivative(const float& Distance, const float& Radius);
    float CalculateDensity(const FVector& Position);
    FVector CalculatePressureForce(const FVector& Position, const int Index);

    TArray<FParticle> Particles;
    FVector MinBounds = FVector::ZeroVector;
    FVector MaxBounds = FVector::ZeroVector;

    // Simulation Settings
    FVector Gravity = FVector(0.0f, 0.0f, -9.81f);
    float PressureAmplifier = 10.f;
    float TargetDensity = 2.f;
    float CollisionDampening = 0.6f;
    float SmoothingRadius = 4.f;
};