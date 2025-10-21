#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

#include "ExternalForceObject.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent, DisplayName="Atlas External Froce Object"))
class PARTICLEBASEDFLUIDSIMULATION_API UExternalForceComponent : public USceneComponent
{
    GENERATED_BODY()

public:
    UExternalForceComponent();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void OnRegister() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="External forces")
	float ForceAmplifier = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="External forces", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float Radius = 3.0f;

private:

	UStaticMesh* DefaultCubeMesh;
	class UInstancedStaticMeshComponent* ExternalForceMesh;

};