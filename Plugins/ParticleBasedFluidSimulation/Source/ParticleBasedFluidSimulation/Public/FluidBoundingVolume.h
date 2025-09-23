// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidSimulationSystem.h"
#include "Templates/UniquePtr.h"
#include "Shaders.h"

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

public:	
	UPROPERTY(VisibleAnywhere, Category = "Bounds")
	class UBoxComponent* Bounds;

	UPROPERTY(VisibleAnywhere, Category = "Particles")
    class UInstancedStaticMeshComponent* ParticleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesX = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesY = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesZ = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Fluid system")
    float MaxSpeedGradient = 30.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fluid system")
	FFluidSimSettings Settings;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TUniquePtr<FFluidSimulationSystem> Simulation;
	TArray<FParticle> Particles;
	TArray<FVector3f> InitialPositions;
private:
	void InitializeParticles();
	void UpdateVolumeBounds();
	void UpdateInstances();
	FLinearColor VelocityToColor(const float& Speed);

	#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	#endif

	UStaticMesh* DefaultSphereMesh;
	const float SphereRadius = 1.0f;
	const float FixedTimeStep = 1.f/60.f;
	float TotalTime = 0.0f;
	bool IsInitialized = false;
};
