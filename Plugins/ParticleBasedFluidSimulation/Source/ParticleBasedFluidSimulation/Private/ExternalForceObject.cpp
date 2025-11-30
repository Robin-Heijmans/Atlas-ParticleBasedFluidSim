#include "ExternalForceObject.h"
#include "Components/InstancedStaticMeshComponent.h"

UExternalForceComponent::UExternalForceComponent() {
    PrimaryComponentTick.bCanEverTick = true;

    ExternalForceMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ExternalForceMesh"));

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

void UExternalForceComponent::BeginPlay() {
    Super::BeginPlay();
}

void UExternalForceComponent::OnRegister()
{
    Super::OnRegister();
    
    ExternalForceMesh->SetupAttachment(this);

    if (ExternalForceMesh && ExternalForceMesh->GetStaticMesh())
    {
        ExternalForceMesh->ClearInstances();

        FTransform InstanceTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1/50.f));
        ExternalForceMesh->AddInstance(InstanceTransform);
        FLinearColor Color = FLinearColor::Red;
        ExternalForceMesh->SetCustomDataValue(0, 0, Color.R, true);
        ExternalForceMesh->SetCustomDataValue(0, 1, Color.G, true);
        ExternalForceMesh->SetCustomDataValue(0, 2, Color.B, true);
    }
}

void UExternalForceComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}