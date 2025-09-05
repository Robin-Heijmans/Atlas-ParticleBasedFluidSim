// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidBoundingVolume.h"

// Sets default values
AFluidBoundingVolume::AFluidBoundingVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AFluidBoundingVolume::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFluidBoundingVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

