// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsTestPC.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
//#include "../../../Plugins/ParticleBasedFluidSimulation/Source/ParticleBasedFluidSimulation/Public/ExternalForceObject.h"
#include "ExternalForceObject.h"

APhysicsTestPC::APhysicsTestPC()
{
    DraggedActor = nullptr;
}

void APhysicsTestPC::BeginPlay()
{
    Super::BeginPlay();

    // Register mapping context at runtime
    if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
        {
            Subsystem->AddMappingContext(InputMapping, 0);
        }
    }
}

void APhysicsTestPC::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
    {
        if (DragAction && ScrollAction && ToggleModeAction)
        {
            EIC->BindAction(DragAction, ETriggerEvent::Started, this, &APhysicsTestPC::OnDragPressed);
            EIC->BindAction(DragAction, ETriggerEvent::Completed, this, &APhysicsTestPC::OnDragReleased);
            EIC->BindAction(DragAction, ETriggerEvent::Triggered, this, &APhysicsTestPC::OnDragTick);
            EIC->BindAction(ScrollAction, ETriggerEvent::Triggered, this, &APhysicsTestPC::OnScrollWheel);
            EIC->BindAction(ToggleModeAction, ETriggerEvent::Started, this, &APhysicsTestPC::OnToggleMode);
        }
    }
}

void APhysicsTestPC::OnDragPressed(const FInputActionValue& Value)
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);
    float MouseX, MouseY;
    GetMousePosition(MouseX, MouseY);
    previousPos = FVector2D(MouseX, MouseY);
    
    if (Cast<AExternalForceObject>(Hit.GetActor())) {
        DraggedActor = Hit.GetActor();
    }
}

void APhysicsTestPC::OnDragReleased(const FInputActionValue& Value)
{
    DraggedActor = nullptr;
}

void APhysicsTestPC::OnDragTick(const FInputActionValue& Value)
{
    if (!bEditorMode) return;

    float MouseX, MouseY;
    GetMousePosition(MouseX, MouseY);
    currentPos = FVector2D(MouseX, MouseY);
    FVector2D deltaPos = currentPos - previousPos;

    FVector CameraUp = PlayerCameraManager->GetActorUpVector();
    FVector CameraRight = PlayerCameraManager->GetActorRightVector();
    //float HorizontalDelta = FVector::DotProduct(Direction, CameraRight);
    //float VerticalDelta = FVector::DotProduct(Direction, CameraUp);

    // Move the actor directly while button is held
    GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, (FString::Printf(TEXT("Mouse position: %f, %f, %f"), CameraRight.X, CameraRight.Y, CameraRight.Z)));
    if (DraggedActor)
    {
        DraggedActor->AddActorWorldOffset((CameraRight * deltaPos.X + CameraUp * -deltaPos.Y) * MovementSpeed);
    }
    previousPos = currentPos;
}

void APhysicsTestPC::OnScrollWheel(const FInputActionValue& Value)
{
    if (!bEditorMode) return;

    float ScrollAmount = Value.Get<float>(); // positive or negative
    if (DraggedActor)
    {
        FVector CameraForward = PlayerCameraManager->GetActorForwardVector();
        DraggedActor->AddActorWorldOffset(CameraForward * ScrollAmount * ScrollSpeed);
    }
}

void APhysicsTestPC::OnToggleMode(const FInputActionValue& Value)
{
    bEditorMode = !bEditorMode;

    if (bEditorMode)
    {
        // Switch to Editor/Drag mode
        bShowMouseCursor = true;
        DefaultMouseCursor = EMouseCursor::Default;

        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        InputMode.SetHideCursorDuringCapture(false);
        SetInputMode(InputMode);

        SetIgnoreLookInput(true);

        UE_LOG(LogTemp, Warning, TEXT("Switched to Drag Mode"));
    }
    else
    {
        // Switch back to Camera mode
        bShowMouseCursor = false;

        FInputModeGameOnly InputMode;
        SetInputMode(InputMode);

        SetIgnoreLookInput(false);

        UE_LOG(LogTemp, Warning, TEXT("Switched to Camera Mode"));
    }
}