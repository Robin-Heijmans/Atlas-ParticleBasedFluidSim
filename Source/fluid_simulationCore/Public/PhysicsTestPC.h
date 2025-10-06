// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"

#include "PhysicsTestPC.generated.h"

/**
 * 
 */
UCLASS()
class FLUID_SIMULATIONCORE_API APhysicsTestPC : public APlayerController
{
	GENERATED_BODY()

public:
    APhysicsTestPC();

protected:
    virtual void BeginPlay() override;
    virtual void SetupInputComponent() override;

    // Input assets (assign these in the editor)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    TObjectPtr<class UInputMappingContext> InputMapping;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    TObjectPtr<class UInputAction> DragAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    TObjectPtr<class UInputAction> ScrollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    TObjectPtr<class UInputAction> ToggleModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    float MovementSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    float ScrollSpeed = 10.0f;
private:
    void OnDragPressed(const FInputActionValue& Value);
    void OnDragReleased(const FInputActionValue& Value);
	void OnDragTick(const FInputActionValue& Value);
	void OnToggleMode(const FInputActionValue& Value);
	void OnScrollWheel(const FInputActionValue& Value);

	bool bEditorMode = false;
    TObjectPtr<class UExternalForceComponent> DraggedForceComponent;

	FVector2D previousPos = FVector2D::ZeroVector;
	FVector2D currentPos = FVector2D::ZeroVector;
	FVector maxStepSize = FVector(0.1f, 0.1f, 0.1f);
};
