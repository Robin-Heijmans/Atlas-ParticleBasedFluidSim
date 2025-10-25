// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidBoundingVolume.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "PhysicsEngine/BodyInstance.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "Engine/World.h"
#include "ExternalForceObject.h"
#include "Engine/OverlapResult.h"

// Sets default values
UFluidBoundingVolumeComponent::UFluidBoundingVolumeComponent()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryComponentTick.bCanEverTick = true;


	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
    Bounds->SetBoxExtent(BoxExtents * Bounds->GetComponentScale(), true);
    Bounds->SetupAttachment(this);
    Bounds->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Bounds->SetCollisionObjectType(ECC_WorldDynamic);
    Bounds->SetCollisionResponseToAllChannels(ECR_Ignore);
    Bounds->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    Bounds->SetGenerateOverlapEvents(true);

    SpawnParticlesBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("SpawnParticleBounds"));
    SpawnParticlesBounds->SetBoxExtent(SpawnBoxExtents, false);
    SpawnParticlesBounds->SetupAttachment(this);
    SpawnParticlesBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SpawnParticlesBounds->SetGenerateOverlapEvents(false);

	ParticleMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ParticleMesh"));
    ParticleMesh->SetupAttachment(this);
    ParticleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

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

    //GenerateParticleBuffers();
}

void UFluidBoundingVolumeComponent::OnRegister()
{
    Super::OnRegister();
    UpdateVolumeBounds();
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (FString::Printf(TEXT("On Register"))));

}

void UFluidBoundingVolumeComponent::OnUnregister() {
    Super::OnUnregister();
}

void UFluidBoundingVolumeComponent::GenerateParticleBuffers()
{
    if(State == EAtlasVolumeState::EMPTY)
        State = EAtlasVolumeState::GENERATE;
}

void UFluidBoundingVolumeComponent::RemoveParticleBuffers()
{
    if(State == EAtlasVolumeState::SIMULATE)
        State = EAtlasVolumeState::RELEASE;
}

void UFluidBoundingVolumeComponent::InitializeParticles()
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
    UpdateVolumeBounds();
    FVector Extent = SpawnParticlesBounds->GetUnscaledBoxExtent();
	FVector WorldScale = SpawnParticlesBounds->GetComponentScale();

	float SpacingX = (Extent.X * 2.f) / FMath::Max(NumParticlesX, 1);
    float SpacingY = (Extent.Y * 2.f) / FMath::Max(NumParticlesY, 1);
    float SpacingZ = (Extent.Z * 2.f) / FMath::Max(NumParticlesZ, 1);
    const float SpacingXOffset = SpacingX * 0.1f;
    const float SpacingYOffset = SpacingY * 0.1f;
    const float SpacingZOffset = SpacingZ * 0.1f;

	const FTransform BoxTransform = SpawnParticlesBounds->GetComponentTransform().GetRelativeTransform(Bounds->GetComponentTransform());
    
	FVector SpawnMin = -Extent + SpawnParticlesBounds->GetRelativeLocation();
    FVector ScaledParticleSize = FVector(ParticleSize)/Bounds->GetComponentScale();
    for (int x = 0; x < NumParticlesX; x++)
    {
        for (int y = 0; y < NumParticlesY; y++)
        {
            for (int z = 0; z < NumParticlesZ; z++)
            {
                FVector LocalPos = SpawnMin + FVector(x * SpacingX + (FMath::FRand() * 2 * SpacingXOffset - SpacingXOffset), 
                                                      y * SpacingY + (FMath::FRand() * 2 * SpacingYOffset - SpacingYOffset),
                                                      z * SpacingZ + (FMath::FRand() * 2 * SpacingZOffset - SpacingZOffset));
				FVector BoundsPos = BoxTransform.TransformPosition(LocalPos);
                Particles.Add(FParticle{BoundsPos});
                MeshPositions.Add(BoundsPos);
                int CurrentNumParticles = Particles.Num();
                if (CurrentNumParticles > PreviousNumParticles) {
                    FTransform InstanceTransform(FRotator::ZeroRotator, BoundsPos, ScaledParticleSize);
                    ParticleMesh->AddInstance(InstanceTransform);
                }
            }
        }
    }

    if (!Simulation) {
	    Simulation = MakeUnique<FFluidSimulationSystem>();
    }
    Extent = Bounds->GetUnscaledBoxExtent();
    FVector LocalMin = -Extent;
	FVector LocalMax = Extent;
	Simulation->InitializeParticles(Particles, LocalMin, LocalMax);
}

void UFluidBoundingVolumeComponent::UpdateVolumeBounds() {
    if (!Simulation) Simulation = MakeUnique<FFluidSimulationSystem>();

    SpawnParticlesBounds->SetRelativeLocation(SpawnBoxOffset);
    SpawnParticlesBounds->SetBoxExtent(SpawnBoxExtents, false);
    Bounds->SetBoxExtent(BoxExtents * Bounds->GetComponentScale(), true);
    FVector Extent = Bounds->GetUnscaledBoxExtent();

    FVector LocalMin = -Extent;
	FVector LocalMax = Extent;
    Simulation->SetVolumeBounds(LocalMin, LocalMax);
}

// Called when the game starts or when spawned
void UFluidBoundingVolumeComponent::BeginPlay()
{
	Super::BeginPlay();
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (FString::Printf(TEXT("Begin play"))));

    if (!IsInitialized) {
        IsInitialized = true;
        InitializeParticles();
        UpdateInstances(true);
        UpdateMaterials();
    }
    
    if (Simulation){
        Simulation->ApplySettings(Settings);
    }
}

// Called every frame
void UFluidBoundingVolumeComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TotalTime += DeltaTime;
	
    if (RunCPU)
    {
        if (FrameCount > SetPositionsCount) FrameCount = 0;

        if (TotalTime >= FixedTimeStep)
        {
            GetExternalForce();
            Simulation->StepSimulation(FixedTimeStep, *Bounds);
            CollisionsCheck();
            Particles = Simulation->GetParticles();
            TotalTime = 0.0f;
            UpdateInstances(false);
            UpdateMaterials();
            FrameCount++;
        }
        //FVector Extent = Bounds->GetUnscaledBoxExtent();
    }
}

void UFluidBoundingVolumeComponent::PostLoad() {
    Super::PostLoad();
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (FString::Printf(TEXT("Post load"))));  

    //UpdateVolumeBounds();

    if (!Simulation) {
	    Simulation = MakeUnique<FFluidSimulationSystem>();
    }
    Simulation->ApplySettings(Settings);
}

void UFluidBoundingVolumeComponent::UpdateMaterials()
{
    if (!DefaultSphereMesh || Particles.Num() == 0) return;
    FTransform WorldTransfom = Bounds->GetComponentTransform();
    for (int32 i = 0; i < Particles.Num(); i++) {
        const FParticle& particle = Particles[i];
        FVector posOffset = WorldTransfom.TransformVector(particle.Position - MeshPositions[i]);

        FLinearColor Color = VelocityToColor(particle.Velocity.Length());

        ParticleMesh->SetCustomDataValue(i, 0, Color.R, true);
        ParticleMesh->SetCustomDataValue(i, 1, Color.G, true);
        ParticleMesh->SetCustomDataValue(i, 2, Color.B, true);
        ParticleMesh->SetCustomDataValue(i, 3, posOffset.X, true);
        ParticleMesh->SetCustomDataValue(i, 4, posOffset.Y, true);
        ParticleMesh->SetCustomDataValue(i, 5, posOffset.Z, true);
    }
}

void UFluidBoundingVolumeComponent::UpdateInstances(const bool AllInstances) {
    if (!DefaultSphereMesh || Particles.Num() == 0) return;
    int32 startIndex = 0;
    int32 endIndex = Particles.Num();
    if (!AllInstances) {
        int numParticlesToUpdate = FMath::CeilToInt(static_cast<float>(Particles.Num()) / SetPositionsCount);
        startIndex = FrameCount * numParticlesToUpdate;
        endIndex = startIndex + numParticlesToUpdate;
    }

    FVector ScaledParticleSize = FVector(ParticleSize)/Bounds->GetComponentScale();
    for (int32 i = startIndex; i < endIndex; i++) {
        if (i >= Particles.Num()) break;
        const FParticle& particle = Particles[i];
        MeshPositions[i] = particle.Position;
        FTransform InstanceTransform(
            FRotator::ZeroRotator,
            particle.Position,
            ScaledParticleSize
        );

        ParticleMesh->UpdateInstanceTransform(i, InstanceTransform, false, true);
    }

    // Apply all pending transform updates in one go
    ParticleMesh->MarkRenderStateDirty();
}

FLinearColor UFluidBoundingVolumeComponent::VelocityToColor(const float& Speed) {
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
void UFluidBoundingVolumeComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) {
    Super::PostEditChangeProperty(PropertyChangedEvent);
    //GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, (FString::Printf(TEXT("Post edit change property"))));    
    //if (Simulation) {
    //    Simulation->ApplySettings(Settings);
    //}
}
#endif

TArray<FVector> UFluidBoundingVolumeComponent::GetVolumeBounds() {
    FVector Extent = Bounds->GetScaledBoxExtent();

    FVector LocalMin = -Extent;
	FVector LocalMax = Extent;
    TArray<FVector> bounds = {LocalMin, LocalMax};
    return bounds;
}

void UFluidBoundingVolumeComponent::GetExternalForce() {
    UWorld* World = GetWorld();
    if (!World) return;

    for (TObjectIterator<UExternalForceComponent> It; It; ++It)
    {
        UExternalForceComponent* ForceComp = *It;
        if (!ForceComp || !ForceComp->GetWorld() || ForceComp->GetWorld() != World)
            continue;

        FVector WorldPos = ForceComp->GetComponentLocation();
        FVector LocalPos = GetComponentTransform().InverseTransformPosition(WorldPos);
        Simulation->ApplyExternalForce(LocalPos, ForceComp->ForceAmplifier, ForceComp->Radius);
    }
}

void UFluidBoundingVolumeComponent::CollisionsCheck() {
    TArray<FOverlapResult> Overlaps = GetCollisionOverlaps();

    for (auto& Overlap : Overlaps)
    {
        UPrimitiveComponent* Comp = Overlap.GetComponent();
        if (!Comp) continue;
        FVector LocalPos = GetComponentTransform().InverseTransformPosition(Comp->GetComponentLocation());

        if (UBoxComponent* Box = Cast<UBoxComponent>(Comp)) {
            Simulation->BoxCollision(*Box, *Bounds, LocalPos, GetWorld());
        }
        else if (USphereComponent* Sphere = Cast<USphereComponent>(Comp)) {
            Simulation->SphereCollision(*Sphere, *Bounds, LocalPos, GetWorld());
        }
        else if (UCapsuleComponent* Capsule = Cast<UCapsuleComponent>(Comp)) {
            Simulation->CapsuleCollision(*Capsule, *Bounds, LocalPos, GetWorld());
        }
        else {
            UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(Comp);
            GEngine->AddOnScreenDebugMessage(3, 5.f, FColor::Yellow, (FString::Printf(TEXT("RIP: Static mesh detected"))));
            if (MeshComp && MeshComp->GetBodyInstance())
            {
                FBodyInstance* Body = MeshComp->GetBodyInstance();
                // To be implemented (complex shapes)
            }
        }
    }
}

TArray<FOverlapResult> UFluidBoundingVolumeComponent::GetCollisionOverlaps() {
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params;
    Params.AddIgnoredComponent(Bounds.Get());

    FVector Center = Bounds->GetComponentLocation();
    FVector Extent = Bounds->GetScaledBoxExtent();
    FQuat Rotation = Bounds->GetComponentQuat();

    GetWorld()->OverlapMultiByChannel(
        Overlaps,
        Center,
        Rotation,
        ECC_PhysicsBody,
        FCollisionShape::MakeBox(Extent),
        Params
    );
    return Overlaps;
}