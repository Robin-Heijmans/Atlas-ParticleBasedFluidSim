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

	virtual void OnUnregister() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	virtual void PostLoad() override;

public:	
	TObjectPtr<class UBoxComponent> Bounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Bounds")
	FVector BoxExtents = FVector(32.0f);

	TObjectPtr<class UBoxComponent> SpawnParticlesBounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Bounds")
	FVector SpawnBoxExtents = FVector(10.f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Bounds")
	FVector SpawnBoxOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Particles", meta = (ClampMin = "1"))
    int NumParticlesX = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Particles", meta = (ClampMin = "1"))
    int NumParticlesY = 5;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Particles", meta = (ClampMin = "1"))
    int NumParticlesZ = 5;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Particles", meta = (ClampMin = "0.0"))
	float ParticleSize = 0.02f;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	FVector3f ExtinctionCoeff = FVector3f(1.f,0.55f,0.35f);
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	float MarchStepSize = 0.02f;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	float LightStepSize = 0.4f;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	float DensityStepSize = 0.5f;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	float DensityMultiplier = 25.f;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	float indexOfRefraction = 1.33f;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	int32 NumRefraction = 4;
    UPROPERTY(EditAnywhere, Category = "Atlas/Particles")
	float SmoothingRadius = 3.0;
	
	UFUNCTION(CallInEditor, Category = "Atlas/Particles")
    void GenerateParticleBuffers();	
    
    UFUNCTION(CallInEditor, Category = "Atlas/Particles")
    void RemoveParticleBuffers();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Atlas/Fluid system")
    float MaxSpeedGradient = 30.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atlas/Fluid system")
	bool RunCPU = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atlas/Fluid system")
	FFluidSimSettings Settings;
	//UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Atlas/Fluid system")
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

	TObjectPtr<UStaticMesh> DefaultSphereMesh;
	TObjectPtr<class UInstancedStaticMeshComponent> ParticleMesh;

	const float SphereRadius = 1.0f;
	const float FixedTimeStep = 1.f/60.f;
	const int SetPositionsCount = 60;
	float TotalTime = 0.0f;
	int FrameCount = 0;
	bool IsInitialized = false;
};