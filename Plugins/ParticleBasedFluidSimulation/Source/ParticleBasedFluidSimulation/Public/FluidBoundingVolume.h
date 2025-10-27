// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "FluidSimulationSystem.h"
#include "Templates/UniquePtr.h"

// Has to be last include in header
#include "FluidBoundingVolume.generated.h"

enum class EAtlasVolumeState
{
	EMPTY = 0,
	RELEASE = 1,
	GENERATE = 2,
	SIMULATE = 3,
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="Atlas Bounding Volume"))
class PARTICLEBASEDFLUIDSIMULATION_API UFluidBoundingVolumeComponent  : public USceneComponent
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	UFluidBoundingVolumeComponent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnRegister() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:	
	UPROPERTY(EditAnywhere, Category = "Bounds")
	class UBoxComponent* Bounds;

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
	float LightStepSize = 0.2f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float DensityStepSize = 0.25f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float DensityMultiplier = 2.f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float indexOfRefraction = 1.33f;
    UPROPERTY(EditAnywhere, Category = "Particles")
	int32 NumRefraction = 4;
    UPROPERTY(EditAnywhere, Category = "Particles")
	float SmoothingRadius = 3.0;

    UFUNCTION(CallInEditor, Category = "Particles")
    void GenerateParticleBuffers();
	
    UFUNCTION(CallInEditor, Category = "Particles")
    void RemoveParticleBuffers();

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
	EAtlasVolumeState State = EAtlasVolumeState::EMPTY;

public:
	TArray<FVector> GetVolumeBounds();
	TArray<FOverlapResult> GetCollisionOverlaps();
private:
	void InitializeParticles();
	void UpdateVolumeBounds();
	void UpdateMaterials();
	void UpdateInstances(const bool AllInstances);
	FLinearColor VelocityToColor(const float& Speed);
	void GetExternalForce();
	void CollisionsCheck();

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	UStaticMesh* DefaultSphereMesh;
	class UInstancedStaticMeshComponent* ParticleMesh;

	const float SphereRadius = 1.0f;
	const float FixedTimeStep = 1.f/60.f;
	const int SetPositionsCount = 60;
	float TotalTime = 0.0f;
	int FrameCount = 0;
	bool IsInitialized = false;
};
