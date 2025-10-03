#include "ExternalForceObject.h"
#include "Components/InstancedStaticMeshComponent.h"

AExternalForceObject::AExternalForceObject() {
    PrimaryActorTick.bCanEverTick = false;

    ExternalForceMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ExternalForceMesh"));
    ExternalForceMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMeshObj(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMeshObj.Succeeded())
    {
        DefaultCubeMesh = CubeMeshObj.Object;
        ExternalForceMesh->SetStaticMesh(DefaultCubeMesh);
        ExternalForceMesh->NumCustomDataFloats = 6;
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> CubeMat(TEXT("/ParticleBasedFluidSimulation/Materials/M_ParticleColor.M_ParticleColor"));
        if (CubeMat.Succeeded())
        {
            ExternalForceMesh->SetMaterial(0, CubeMat.Object);
        }
    }
}

void AExternalForceObject::BeginPlay() {
    Super::BeginPlay();
}

void AExternalForceObject::OnConstruction(const FTransform& Transform) {
    Super::OnConstruction(Transform);
}

void AExternalForceObject::Tick(float DeltaTime) {
    Super::Tick(DeltaTime);
}