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

    InitializeParticles();
    UpdateInstances();

    // creating rendertarget texture for the compute shader test
    RenderTest = UKismetRenderingLibrary::CreateRenderTarget2D(this, 1024, 1024, RTF_RGBA8);
}

void AFluidBoundingVolume::InitializeParticles()
{
    Particles.Empty();

    FVector Extent = Bounds->GetScaledBoxExtent();
	FVector WorldScale = Bounds->GetComponentScale();

	float SpacingX = (NumParticlesX > 1) ? ((2 * Extent.X) / (NumParticlesX - 1) / WorldScale.X) : 0.f;
    float SpacingY = (NumParticlesY > 1) ? ((2 * Extent.Y) / (NumParticlesY - 1) / WorldScale.Y): 0.f;
    float SpacingZ = (NumParticlesZ > 1) ? ((2 * Extent.Z) / (NumParticlesZ - 1) / WorldScale.Z): 0.f;

	const FTransform BoxTransform = Bounds->GetComponentTransform();
    const FVector LocalMin = -Extent / WorldScale;

    for (int x = 0; x < NumParticlesX; x++)
    {
        for (int y = 0; y < NumParticlesY; y++)
        {
            for (int z = 0; z < NumParticlesZ; z++)
            {
                FVector LocalPos = LocalMin + FVector(x * SpacingX, y * SpacingY, z * SpacingZ);
				FVector WorldPos = BoxTransform.TransformPosition(LocalPos);
                Particles.Add(FParticle{LocalPos,FVector::ZeroVector, 1.0f});
            }
        }
    }

	Simulation = MakeUnique<FFluidSimulationSystem>();
	Simulation->InitializeParticles(Particles);
}


// Called when the game starts or when spawned
void AFluidBoundingVolume::BeginPlay()
{
	Super::BeginPlay();
	InitializeParticles();
}

// Called every frame
void AFluidBoundingVolume::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	Simulation->StepSimulation(DeltaTime);
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
            FTransform InstanceTransform(FRotator::ZeroRotator, Particle.Position, FVector(SphereRadius)); // scale down spheres
            ParticleMesh->AddInstance(InstanceTransform);
        }
    }
}

void AFluidBoundingVolume::TestDispatch()
{
    UComputeShaderLibrary::ExecuteRTComputeShader(RenderTest, EyePosition);
}