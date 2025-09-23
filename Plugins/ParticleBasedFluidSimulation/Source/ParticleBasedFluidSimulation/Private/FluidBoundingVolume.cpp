// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidBoundingVolume.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "Engine/World.h"

#include "Shaders.h"

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
        ParticleMesh->NumCustomDataFloats = 3;
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
        UpdateInstances();
    }
}

void AFluidBoundingVolume::InitializeParticles()
{
    Particles.Empty();
    InitialPositions.Empty();
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
                InitialPositions.Add(FVector3f(LocalPos));
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
	if (TotalTime >= FixedTimeStep)
	{
		Simulation->StepSimulation(FixedTimeStep);
        Particles = Simulation->GetParticles();
        ENQUEUE_RENDER_COMMAND(DispatchParticleSim)(
            [this](FRHICommandListImmediate& RHICmdList)
            {
                FRDGBuilder GraphBuilder(RHICmdList);

                // Prepare pass parameters with your persistent UAVs/SRVs
                ParticleParams.CreateBuffers(GraphBuilder, InitialPositions);

                int NumParticles = ParticleParams.PassParameters->NumParticles;
                FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumParticles);
                FRDGBufferRef Buffer = GraphBuilder.CreateBuffer(Desc, TEXT("Predicted Positions"));
                TArray<FVector3f> Positions; 
                Positions.Init(FVector3f(0,0,0), NumParticles);
                GraphBuilder.QueueBufferUpload(Buffer, Positions.GetData(), NumParticles * sizeof(FVector3f));
                FRDGBufferUAVRef UAV = GraphBuilder.CreateUAV(Buffer);
                ParticleParams.PassParameters->PredictedPositions = UAV;

                // Run your simulation dispatch
                ParticleParams.Dispatch(GraphBuilder, GetGlobalShaderMap(GMaxRHIFeatureLevel));

                GraphBuilder.AddPass(
                    RDG_EVENT_NAME("ReadbackPredictedPositions"),
                    ERDGPassFlags::NeverCull,
                    [PredictedBuffer = Buffer, NumParticles](FRHICommandListImmediate& RHICmdList)
                    {
                        FRHIGPUBufferReadback Readback(TEXT("PredictedPositionsReadback"));
                        Readback.EnqueueCopy(RHICmdList, PredictedBuffer->GetRHI(), sizeof(FVector3f) * NumParticles);

                        if (Readback.IsReady()) {
                        FVector3f* Data = (FVector3f*)Readback.Lock(NumParticles * sizeof(FVector3f));
                        GEngine->AddOnScreenDebugMessage(0, 5.f, FColor::Green,
                                FString::Printf(TEXT("First particle: X=%f Y=%f Z=%f"), Data[0].X, Data[0].Y, Data[0].Z));
                        Readback.Unlock();
                        }
                    }
                );
                GraphBuilder.Execute();
            }
        );
        GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Red, FString::Printf(TEXT("First particle: X=%f Y=%f Z=%f"), InitialPositions[0].X, InitialPositions[0].Y, InitialPositions[0].Z));
		TotalTime = 0.0f;
	}
	
	UpdateInstances();
}

void AFluidBoundingVolume::UpdateInstances()
{
    if (!DefaultSphereMesh || Particles.Num() == 0) return;

    for (int32 i = 0; i < Particles.Num(); i++)
    {
        const FParticle& particle = Particles[i];
        FTransform InstanceTransform(
            FRotator::ZeroRotator,
            particle.Position,
            FVector(SphereRadius / 50.0f)
        );
        FLinearColor Color = VelocityToColor(particle.Velocity.Length());

        ParticleMesh->SetCustomDataValue(i, 0, Color.R, true);
        ParticleMesh->SetCustomDataValue(i, 1, Color.G, true);
        ParticleMesh->SetCustomDataValue(i, 2, Color.B, true);
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
        UpdateInstances();
    }
    // Update simulation only when values are changed in editor
    Simulation->ApplySettings(Settings);
}
#endif