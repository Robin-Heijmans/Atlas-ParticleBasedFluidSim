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

    for (TActorIterator<AFluidBoundingVolume> FluidVolumes(World); FluidVolumes; ++FluidVolumes)
    {
        // Update UBOs continously

        // Initialize ParticleBuffers
        if(!FluidVolumes->HasParticles)
        {
            TArray<USceneComponent*> Children;
            FluidVolumes->GetRootComponent()->GetChildrenComponents(true, Children);
            for(USceneComponent* Child : Children)
            {
                UParticleBuffers* ParticleBuffers = nullptr;
                ParticleBuffers = dynamic_cast<UParticleBuffers*>(Child);
                if(ParticleBuffers && ParticleBuffers->bInitialized)
                {
                    ParticleBuffers->UnregisterComponent(); // doesnt working, but doesnt break anything either
                    
                }
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