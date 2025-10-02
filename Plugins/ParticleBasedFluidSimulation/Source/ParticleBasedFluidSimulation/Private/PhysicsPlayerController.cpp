// Fill out your copyright notice in the Description page of Project Settings.


#include "PhysicsPlayerController.h"

APhysicsPlayerController::APhysicsPlayerController() {

}

void APhysicsPlayerController::BeginPlay() {
    Super::BeginPlay();
    GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Blue, (FString::Printf(TEXT("Hello"))));
}

void APhysicsPlayerController::OnConstruction(const FTransform& Transform) {
    Super::OnConstruction(Transform);
}

void APhysicsPlayerController::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);

    if (bDragging)
    {
        FHitResult Hit;
        GetHitResultUnderCursor(ECC_Visibility, false, Hit);

        if (Hit.bBlockingHit)
        {
            if (DraggedActor)
            {
                DraggedActor->SetActorLocation(Hit.Location);
            }
        }
    }
}

void APhysicsPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    InputComponent->BindAction("LeftClick", IE_Pressed, this, &APhysicsPlayerController::OnClick);
    InputComponent->BindAction("LeftClick", IE_Released, this, &APhysicsPlayerController::OnRelease);
}

void APhysicsPlayerController::OnClick()
{
    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (Hit.GetActor())
    {
        DraggedActor = Hit.GetActor();
        bDragging = true;
    }
}

void APhysicsPlayerController::OnRelease()
{
    bDragging = false;
    DraggedActor = nullptr;
}