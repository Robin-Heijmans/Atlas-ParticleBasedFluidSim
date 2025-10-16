// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidSimulationSystem.h"
#include "Templates/UniquePtr.h"

// Has to be last include in header
#include "FluidBoundingVolume.generated.h"

UCLASS()
class PARTICLEBASEDFLUIDSIMULATION_API AFluidBoundingVolume : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFluidBoundingVolume();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:	
	UPROPERTY(EditAnywhere, Category = "Bounds")
	class UBoxComponent* Bounds;

	UPROPERTY(VisibleAnywhere, Category = "Particles")
    class UInstancedStaticMeshComponent* ParticleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesX = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesY = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesZ = 5;

    UPROPERTY(EditAnywhere, Category = "Particles")
	FVector3f ExtinctionCoeff = FVector3f(1.f,0.55f,0.35f);
    UPROPERTY(EditAnywhere, Category = "Particles")
	float MarchStepSize = 0.02f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float LightStepSize = 0.4f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float DensityStepSize = 0.5f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float DensityMultiplier = 25.f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float indexOfRefraction = 1.33f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	int32 NumRefraction = 4;

    UFUNCTION(CallInEditor, Category = "Particles")
    void GenerateParticleBuffers();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid system")
    float MaxSpeedGradient = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid system")
	FFluidSimSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid system")
	bool RunCPU = true;

	TUniquePtr<FFluidSimulationSystem> Simulation;
	TArray<FParticle> Particles;
	TArray<FVector3f> InitialPositions;
	TArray<FVector> MeshPositions;
    bool HasParticles = true;

public:
	TArray<FVector> GetVolumeBounds();
private:
	void InitializeParticles();
	void UpdateVolumeBounds();
	void UpdateMaterials();
	void UpdateInstances(const bool AllInstances);
	FLinearColor VelocityToColor(const float& Speed);

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	UStaticMesh* DefaultSphereMesh;
	const float SphereRadius = 1.0f;
	const float FixedTimeStep = 1.f/60.f;
	const int SetPositionsCount = 60;
	float TotalTime = 0.0f;
	int FrameCount = 0;
	bool IsInitialized = false;
};
