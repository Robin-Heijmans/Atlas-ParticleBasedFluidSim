// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidBoundingVolume.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "Engine/World.h"
#include "ExternalForceObject.h"

// Sets default values
AFluidBoundingVolume::AFluidBoundingVolume()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    RootComponent = Bounds;

	ParticleMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ParticleMesh"));
    ParticleMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshObj(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMeshObj.Succeeded())
    {
        DefaultSphereMesh = SphereMeshObj.Object;
        ParticleMesh->SetStaticMesh(DefaultSphereMesh);
        ParticleMesh->NumCustomDataFloats = 6;
        static ConstructorHelpers::FObjectFinder<UMaterialInterface> ParticleMat(TEXT("/ParticleBasedFluidSimulation/Materials/M_ParticleColor.M_ParticleColor"));
        if (ParticleMat.Succeeded())
        {
            ParticleMesh->SetMaterial(0, ParticleMat.Object);
        }
    }
}

void AFluidBoundingVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateVolumeBounds();
    if (!IsInitialized) {
        InitializeParticles();
        UpdateInstances(true);
        UpdateMaterials();
    }
}

void AFluidBoundingVolume::GenerateParticleBuffers()
{
    HasParticles = false;
}

void AFluidBoundingVolume::InitializeParticles()
{
    Particles.Empty();
    MeshPositions.Empty();
    const int TotalNumParticles = NumParticlesX * NumParticlesY * NumParticlesZ;
    const int PreviousNumParticles = ParticleMesh->GetNumInstances();

    if (PreviousNumParticles > TotalNumParticles) {
        for (int i = PreviousNumParticles - 1; i >= TotalNumParticles; i--) {
            ParticleMesh->RemoveInstance(i);
        }
    }

    FVector Extent = Bounds->GetScaledBoxExtent();
	FVector WorldScale = Bounds->GetComponentScale();
	FVector AdjustedExtent = Extent - FVector(SphereRadius) * WorldScale * 5.0f;

	float SpacingX = SphereRadius*2.1f;
    float SpacingY = SphereRadius*2.1f;
    float SpacingZ = SphereRadius*2.1f;

	const FTransform BoxTransform = Bounds->GetComponentTransform();
    FVector LocalMin = -Extent / WorldScale;
	FVector LocalMax = Extent / WorldScale;
	FVector SpawnMin = -AdjustedExtent / WorldScale;
    for (int x = 0; x < NumParticlesX; x++)
    {
        for (int y = 0; y < NumParticlesY; y++)
        {
            for (int z = 0; z < NumParticlesZ; z++)
            {
                FVector LocalPos = SpawnMin + FVector(x * SpacingX, y * SpacingY, z * SpacingZ);
				//FVector WorldPos = BoxTransform.TransformPosition(LocalPos);
                Particles.Add(FParticle{LocalPos});
                MeshPositions.Add(LocalPos);
                int CurrentNumParticles = Particles.Num();
                if (CurrentNumParticles > PreviousNumParticles) {
                    FTransform InstanceTransform(FRotator::ZeroRotator, LocalPos, FVector(SphereRadius/50.0f));
                    ParticleMesh->AddInstance(InstanceTransform);
                }
            }
        }
    }

    if (!Simulation) {
	    Simulation = MakeUnique<FFluidSimulationSystem>();
    }
	Simulation->InitializeParticles(Particles, LocalMin, LocalMax);
}

void AFluidBoundingVolume::UpdateVolumeBounds() {
    if (!Simulation) return;
    FVector Extent = Bounds->GetScaledBoxExtent();
	FVector WorldScale = Bounds->GetComponentScale();

    FVector LocalMin = -Extent / WorldScale;
	FVector LocalMax = Extent / WorldScale;
    Simulation->SetVolumeBounds(LocalMin, LocalMax);
}

// Called when the game starts or when spawned
void AFluidBoundingVolume::BeginPlay()
{
	Super::BeginPlay();
	InitializeParticles();
    IsInitialized = true;
    if (Simulation){
        Simulation->ApplySettings(Settings);
    }
}

// Called every frame
void AFluidBoundingVolume::Tick(float DeltaTime)
{
    
	TotalTime += DeltaTime;
	Super::Tick(DeltaTime);

    if (RunCPU)
    {
        if (FrameCount > SetPositionsCount) FrameCount = 0;

        if (TotalTime >= FixedTimeStep)
        {
            GetExternalForce();
            Simulation->StepSimulation(FixedTimeStep);
            Particles = Simulation->GetParticles();
            TotalTime = 0.0f;
            UpdateInstances(false);
            UpdateMaterials();
            FrameCount++;
        }
    }
}

void AFluidBoundingVolume::UpdateMaterials()
{
    if (!DefaultSphereMesh || Particles.Num() == 0) return;
    FVector WorldScale = Bounds->GetComponentScale();
    for (int32 i = 0; i < Particles.Num(); i++) {
        const FParticle& particle = Particles[i];
        FVector posOffset = (particle.Position - MeshPositions[i]) * WorldScale;

        FLinearColor Color = VelocityToColor(particle.Velocity.Length());

        ParticleMesh->SetCustomDataValue(i, 0, Color.R, true);
        ParticleMesh->SetCustomDataValue(i, 1, Color.G, true);
        ParticleMesh->SetCustomDataValue(i, 2, Color.B, true);
        ParticleMesh->SetCustomDataValue(i, 3, posOffset.X, true);
        ParticleMesh->SetCustomDataValue(i, 4, posOffset.Y, true);
        ParticleMesh->SetCustomDataValue(i, 5, posOffset.Z, true);
    }
}

void AFluidBoundingVolume::UpdateInstances(const bool AllInstances) {
    if (!DefaultSphereMesh || Particles.Num() == 0) return;
    int32 startIndex = 0;
    int32 endIndex = Particles.Num();
    if (!AllInstances) {
        int numParticlesToUpdate = FMath::CeilToInt(static_cast<float>(Particles.Num()) / SetPositionsCount);
        startIndex = FrameCount * numParticlesToUpdate;
        endIndex = startIndex + numParticlesToUpdate;
    }
    
    for (int32 i = startIndex; i < endIndex; i++) {
        if (i >= Particles.Num()) break;
        const FParticle& particle = Particles[i];
        MeshPositions[i] = particle.Position;
        FTransform InstanceTransform(
            FRotator::ZeroRotator,
            particle.Position,
            FVector(SphereRadius / 50.0f)
        );

        ParticleMesh->UpdateInstanceTransform(i, InstanceTransform, false, true);
    }

    // Apply all pending transform updates in one go
    ParticleMesh->MarkRenderStateDirty();
}

FLinearColor AFluidBoundingVolume::VelocityToColor(const float& Speed) {
    float Alpha = FMath::Clamp(Speed / MaxSpeedGradient, 0.0f, 1.0f);

    if (Alpha < 0.5f) // interpolate from blue to green
    {
        float LocalAlpha = Alpha / 0.5f;
        FLinearColor StartColor(0.f, 0.f, 0.5f);
        FLinearColor MidColor(0.f, 1.f, 0.f);
        return FLinearColor::LerpUsingHSV(StartColor, MidColor, LocalAlpha);
    }
    else // interpolate from green to red
    {
        float LocalAlpha = (Alpha - 0.5f) / 0.5f;
        FLinearColor MidColor(0.f, 1.f, 0.f);
        FLinearColor EndColor(1.f, 0.f, 0.f);
        return FLinearColor::LerpUsingHSV(MidColor, EndColor, LocalAlpha);
    }
}

#if WITH_EDITOR
void AFluidBoundingVolume::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    const FName PropertyName = PropertyChangedEvent.GetPropertyName();
    if (PropertyName == GET_MEMBER_NAME_CHECKED(AFluidBoundingVolume, NumParticlesX) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AFluidBoundingVolume, NumParticlesY) ||
        PropertyName == GET_MEMBER_NAME_CHECKED(AFluidBoundingVolume, NumParticlesZ))
    {
        InitializeParticles();
        UpdateInstances(true);
        UpdateMaterials();
    }
    // Update simulation only when values are changed in editor
    Simulation->ApplySettings(Settings);
}
#endif

TArray<FVector> AFluidBoundingVolume::GetVolumeBounds() {
    FVector Extent = Bounds->GetScaledBoxExtent();
	FVector WorldScale = Bounds->GetComponentScale();

    FVector LocalMin = -Extent / WorldScale;
	FVector LocalMax = Extent / WorldScale;
    TArray<FVector> bounds = {LocalMin, LocalMax};
    return bounds;
}

void AFluidBoundingVolume::GetExternalForce() {
    TArray<AExternalForceObject*> FoundForces;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AExternalForceObject::StaticClass(), (TArray<AActor*>&)FoundForces);

    for (auto& actor : FoundForces) {
        FVector localLoc = GetActorTransform().InverseTransformPosition(actor->GetActorLocation());
        //GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, (FString::Printf(TEXT("External force position: %f, %f, %f"), localLoc.X, localLoc.Y, localLoc.Z)));
        Simulation->ApplyExternalForce(localLoc, actor->ForceAmplifier, actor->Radius);
    }
}