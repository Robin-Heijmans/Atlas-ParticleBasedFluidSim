#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "PostProcess/PostProcessInputs.h"
#include "Misc/Optional.h"
#include "GameFramework/Actor.h"
#include "RHICommandList.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UnifiedBuffer.h"	
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"

#include "FluidBoundingVolume.h"


namespace 
{
	TAutoConsoleVariable<int32> CVarRendering(
		TEXT("Atlas.Rendering"),
		0,
		TEXT("Enable Fluid Rendering \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
        
	TAutoConsoleVariable<int32> CVarSimulation(
		TEXT("Atlas.Simulation"),
		0,
		TEXT("Enable Fluid Simulation \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
}

FFluidExtention::FFluidExtention(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) 
{
	UE_LOG(LogTemp, Log, TEXT("Fluid: Custom SceneViewExtension registered"));
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
        // Check for new volumes, that need particles
    UWorld* World = InViewFamily.Scene->GetWorld(); 
    if(World == nullptr) return;

    // Advance simulation
    #if WITH_EDITOR
        if(CVarSimulation.GetValueOnRenderThread() == 1) 
        {
            TotalTime += World->GetDeltaSeconds();
        }
    #else
        TotalTime += World->GetDeltaSeconds();
    #endif

    FFluidVolume FluidVolume;
    FFluidVolumeLocal VolumeBounds;
    FFluidEnvironment FluidEnvironment;

    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (!Actor) continue;

        TArray<UFluidBoundingVolumeComponent*> FluidComponents;
        Actor->GetComponents<UFluidBoundingVolumeComponent>(FluidComponents);


        for (UFluidBoundingVolumeComponent* FluidComp : FluidComponents)
        {
            // Update UBOs continously
            TArray<FVector> bounds = FluidComp->GetVolumeBounds();
            VolumeBounds.MinBounds = FVector3f(bounds[0]);
            VolumeBounds.MaxBounds = FVector3f(bounds[1]);
            
            FluidVolume.BoundsPosition = FVector3f(FluidComp->Bounds->GetComponentLocation());
            FluidVolume.BoundsSize = FVector3f(FluidComp->Bounds->GetScaledBoxExtent());
            
            FTransform Cube(FluidComp->Bounds->GetComponentRotation(), FluidComp->Bounds->GetComponentLocation(), FluidComp->Bounds->GetComponentScale());

            FluidEnvironment.CubeLocalToWorld = FMatrix44f(Cube.ToMatrixWithScale());
            FluidEnvironment.CubeWorldToLocal = FMatrix44f(Cube.ToMatrixWithScale().Inverse());

            FluidEnvironment.ExtinctionCoeff = FVector3f(FluidComp->ExtinctionCoeff);
            FluidEnvironment.MarchStepSize = FluidComp->MarchStepSize;
            FluidEnvironment.LightStepSize = FluidComp->LightStepSize;
            FluidEnvironment.DensityStepSize = FluidComp->DensityStepSize;
            FluidEnvironment.DensityMultiplier = FluidComp->DensityMultiplier;
            FluidEnvironment.indexOfRefraction = FluidComp->indexOfRefraction;
            FluidEnvironment.NumRefractions = FluidComp->NumRefraction;

            UBFluidEnvironment = TUniformBufferRef<FFluidEnvironment>::CreateUniformBufferImmediate(FluidEnvironment, EUniformBufferUsage::UniformBuffer_SingleFrame);  
            UBFluidBounds = TUniformBufferRef<FFluidVolumeLocal>::CreateUniformBufferImmediate(VolumeBounds, EUniformBufferUsage::UniformBuffer_SingleFrame);  
            UBFluidVolume = TUniformBufferRef<FFluidVolume>::CreateUniformBufferImmediate(FluidVolume, EUniformBufferUsage::UniformBuffer_SingleFrame);  
        
            // Initialize ParticleBuffers
            if(!FluidComp->HasParticles)
            {   
                TArray<USceneComponent*> Children;
                FluidComp->GetChildrenComponents(true, Children);
                for(USceneComponent* Child : Children)
                {
                    UParticleBuffers* ParticleBuffers = nullptr;
                    ParticleBuffers = dynamic_cast<UParticleBuffers*>(Child);
                    if(ParticleBuffers && ParticleBuffers->bInitialized)
                    {
                        ParticleBuffers->UnregisterComponent(); // doesnt working, but doesnt break anything either
                        
                    }

                    const uint32 NumParticles = FluidComp->NumParticlesX * FluidComp->NumParticlesY * FluidComp->NumParticlesZ;
                    if(NumParticles > 5000 || NumParticles == 0)     
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Illegal NumParticles: %d"), NumParticles);
                        continue;
                    }

                    UParticleBuffers* NewParticleBuffers = NewObject<UParticleBuffers>(FluidComp,UParticleBuffers::StaticClass(), TEXT("Particle Buffers"));

                    NewParticleBuffers->RegisterComponent();
                    NewParticleBuffers->AttachToComponent(FluidComp, FAttachmentTransformRules::KeepRelativeTransform);

                    // Allocate Particle Buffer
                    NewParticleBuffers->Initialize(UBFluidBounds, FluidComp);
                    NewParticleBuffers->SimulationSettings.DeltaTime = FixedTimeStep;
                    FluidComp->HasParticles = true;
                    return;
                }

                const uint32 NumParticles = FluidComp->NumParticlesX * FluidComp->NumParticlesY * FluidComp->NumParticlesZ;
                if(NumParticles > 5000 || NumParticles == 0)     
                {
                    UE_LOG(LogTemp, Warning, TEXT("Illegal NumParticles: %d"), NumParticles);
                    continue;
                }

                UParticleBuffers* ParticleBuffers = NewObject<UParticleBuffers>(FluidComp,UParticleBuffers::StaticClass(), TEXT("Particle Buffers"));

                ParticleBuffers->RegisterComponent();
                ParticleBuffers->AttachToComponent(FluidComp, FAttachmentTransformRules::KeepRelativeTransform);

                // Allocate Particle Buffer
                ParticleBuffers->Initialize(UBFluidBounds, FluidComp);
                ParticleBuffers->SimulationSettings.DeltaTime = FixedTimeStep;
                FluidComp->HasParticles = true;
                return;
            }

            const uint32 NumParticles = FluidVolumes->NumParticlesX * FluidVolumes->NumParticlesY * FluidVolumes->NumParticlesZ;
            if(NumParticles > 5000 || NumParticles == 0)     
            {
                UE_LOG(LogTemp, Warning, TEXT("Illegal NumParticles: %d"), NumParticles);
                continue;
            }

            UParticleBuffers* ParticleBuffers = NewObject<UParticleBuffers>(*FluidVolumes,UParticleBuffers::StaticClass(), TEXT("Particle Buffers"));

            ParticleBuffers->RegisterComponent();
            ParticleBuffers->AttachToComponent(FluidVolumes->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

            // Allocate Particle Buffer
            ParticleBuffers->Initialize(*FluidVolumes);
            ParticleBuffers->SimulationSettings.DeltaTime = FixedTimeStep;
            FluidVolumes->HasParticles = true;
            return;
        }
    }
}

void FFluidExtention::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) 
{  
    // Simulate
    if (CVarSimulation.GetValueOnRenderThread() == 0) return;

    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(!ParticleBuffers->bInitialized) continue;
        ParticleBuffers->Register(GraphBuilder);
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

        // Particle Simlation
        if(TotalTime > FixedTimeStep)
        {
            ParticleBuffers->DispatchFluidMath(GraphBuilder, GlobalShaderMap);

            TotalTime = 0.f;
        }
    }
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) 
{
    // Render
    if (CVarRendering.GetValueOnRenderThread() == 0) return;

    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());
    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(!ParticleBuffers->bInitialized) continue;
        FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;
        ParticleBuffers->DispatchFluidRender(GraphBuilder, GlobalShaderMap, SceneColor, InView);
    }
}