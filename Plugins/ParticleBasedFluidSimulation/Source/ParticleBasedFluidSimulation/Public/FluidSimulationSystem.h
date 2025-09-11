#pragma once

#include "CoreMinimal.h"
#include "FluidParticle.h"
#include "SpatialLookup.h"

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
    void UpdateSpatialLookup(const float& Radius);
    float ConvertDensityToPressure(const float& Density);
    float CalculateSharedPressure(const float& DensityA, const float& DensityB);
    float SmoothingKernel(const float& Distance, const float& Radius);
    float SmoothingKernelDerivative(const float& Distance, const float& Radius);
    float CalculateDensity(const FVector& Position);
    FVector CalculatePressureForce(const FVector& Position, const int Index);
    FIntVector PositionToCellCoords(const FVector& Position, const float& Radius);
    uint32 HashCell(const FIntVector& CellCoords);
    uint32 GetKeyFromHash(const uint32& Hash);

    TArray<FParticle> Particles;
    TArray<FSpatialLookupEntry> SpatialLookup;
    TArray<uint32> StartIndices;
    FVector MinBounds = FVector::ZeroVector;
    FVector MaxBounds = FVector::ZeroVector;

    // Simulation Settings
    FVector Gravity = FVector(0.0f, 0.0f, -9.81f);
    float PressureAmplifier = 10.f;
    float TargetDensity = 2.f;
    float CollisionDampening = 0.6f;
    float SmoothingRadius = 4.f;

    uint32 TableSize = 0; 
    const uint32 HashKey1 = 467;
    const uint32 HashKey2 = 1997;
    const uint32 HashKey3 = 57847;
};