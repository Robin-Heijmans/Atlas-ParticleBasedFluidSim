// Fill out your copyright notice in the Description page of Project Settings.


#include "FluidBoundingVolume.h"
#include "Components/BoxComponent.h"
#include "Components/InstancedStaticMeshComponent.h"

#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "ShaderInterface/ComputeTest.h"

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
    }
    
}

void AFluidBoundingVolume::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
    UpdateVolumeBounds();
    if (!IsInitialized) {
        InitializeParticles();
        IsInitialized = true;
        UpdateInstances();
    }
    // creating rendertarget texture for the compute shader test
    RenderTest = UKismetRenderingLibrary::CreateRenderTarget2D(this, 1024, 1024, RTF_RGBA8);
}

void AFluidBoundingVolume::InitializeParticles()
{
    Particles.Empty();

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
            }
        }
    }

	Simulation = MakeUnique<FFluidSimulationSystem>();
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
		TotalTime = 0.0f;
	}
	
	UpdateInstances();
    TestDispatch();
}

void AFluidBoundingVolume::UpdateInstances()
{
    ParticleMesh->ClearInstances();

    if (DefaultSphereMesh)
    {
        for (const FParticle& Particle : Particles)
        {
            FTransform InstanceTransform(FRotator::ZeroRotator, Particle.Position, FVector(SphereRadius/50.0f)); // 50.0f is basic sphere mesh radius.
            ParticleMesh->AddInstance(InstanceTransform);
        }
    }
}

void AFluidBoundingVolume::TestDispatch()
{
    UComputeShaderLibrary::ExecuteRTComputeShader(RenderTest, EyePosition);
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