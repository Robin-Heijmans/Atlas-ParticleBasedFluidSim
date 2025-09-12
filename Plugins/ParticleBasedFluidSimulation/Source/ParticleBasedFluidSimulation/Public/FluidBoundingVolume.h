// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FluidSimulationSystem.h"
#include "Templates/UniquePtr.h"
#include "ComputeLibrary.h"
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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RenderTest")
    class UTextureRenderTarget2D* RenderTest;

	UPROPERTY(VisibleAnywhere, Category = "Particles")
    class UInstancedStaticMeshComponent* ParticleMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesX = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesY = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Particles")
    int NumParticlesZ = 5;

    UPROPERTY()
    TArray<FParticle> Particles;

	UFUNCTION(CallInEditor, Category = "RenderTest")
	void TestDispatch();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	TUniquePtr<FFluidSimulationSystem> Simulation;
private:
	void InitializeParticles();
	void UpdateInstances();

	UComputeShaderLibrary CTShaderLib;

	UStaticMesh* DefaultSphereMesh;
	const float SphereRadius = 0.05f;
};
