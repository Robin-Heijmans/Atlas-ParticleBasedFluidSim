#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "ExternalForceObject.generated.h"

UCLASS()
class PARTICLEBASEDFLUIDSIMULATION_API AExternalForceObject : public AActor
{
    GENERATED_BODY()

public:
    AExternalForceObject();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void OnConstruction(const FTransform& Transform) override;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="External forces")
	float ForceAmplifier = 0.0f;

private:

	UStaticMesh* DefaultCubeMesh;
	class UInstancedStaticMeshComponent* ExternalForceMesh;

};