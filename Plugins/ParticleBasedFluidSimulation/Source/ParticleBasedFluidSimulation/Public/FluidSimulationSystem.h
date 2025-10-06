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
    FVector Gravity = FVector(0.0f, 0.0f, -98.1f);
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float PressureAmplifier = 100.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float TargetDensity = 3.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float CollisionDampening = 0.6f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float SmoothingRadius = 4.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid")
    float ViscosityStrength = 1.f;
};

class FFluidSimulationSystem
{
public:
    FFluidSimulationSystem();
    void InitializeParticles(TArray<FParticle>& InParticles, FVector& MinB, FVector& MaxB);
    void SetVolumeBounds(FVector& MinB, FVector& MaxB);
    void StepSimulation(float DeltaTime);
    void ApplySettings(FFluidSimSettings& settings);
    void ApplyExternalForce(const FVector& Location, const float& ForceAmplifier, const float& radius);
    void CheckBoxCollision(const FVector& Location);
    void CheckSphereCollision(const FVector& Location);
    void CheckCapsuleCollision(const FVector& Location);

    const TArray<FParticle>& GetParticles() const { return Particles; }
private:
    void ResolveCollisions(FParticle& particle);
    void UpdateSpatialLookup(const float& Radius);
    float ConvertDensityToPressure(const float& Density);
    float CalculateSharedPressure(const float& DensityA, const float& DensityB);
    float SmoothingKernel(const float& Distance, const float& Radius);
    float SmoothingKernelDerivative(const float& Distance, const float& Radius);
    float SmoothingKernelViscosity(const float& Distance, const float& Radius);
    float CalculateDensity(const FVector& Position, const int Index);
    FVector CalculatePressureForce(const FVector& Position, const int Index);
    FVector CalculateViscosityForce(const FVector& Position, const int Index);
    FIntVector PositionToCellCoords(const FVector& Position, const float& Radius);
    uint32 HashCell(const FIntVector& CellCoords);
    uint32 GetKeyFromHash(const uint32& Hash);

    TArray<FParticle> Particles;
    TArray<FVector> ExternalForces;
    TArray<FSpatialLookupEntry> SpatialLookup;
    TArray<uint32> StartIndices;
    FVector MinBounds = FVector::ZeroVector;
    FVector MaxBounds = FVector::ZeroVector;

    // Simulation Settings
    FVector Gravity = FVector(0.0f, 0.0f, -98.1f);
    float PressureAmplifier = 100.f;
    float TargetDensity = 3.f;
    float CollisionDampening = 0.6f;
    float SmoothingRadius = 4.f;
    float ViscosityStrength = 1.f;
    float Sven = 0.2f;

    uint32 TableSize = 0; 
    const uint32 HashKey1 = 467;
    const uint32 HashKey2 = 1997;
    const uint32 HashKey3 = 57847;
};