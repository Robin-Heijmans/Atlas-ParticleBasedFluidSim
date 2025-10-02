// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "PhysicsPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class PARTICLEBASEDFLUIDSIMULATION_API APhysicsPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	APhysicsPlayerController();
protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupInputComponent() override;
public:
	void OnClick();
	void OnRelease();

	bool bDragging = false;
	TObjectPtr<class AActor> DraggedActor = nullptr;
};
