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
#include "Components/BoxComponent.h"

#include "FluidBoundingVolume.h"


namespace 
{
	TAutoConsoleVariable<int32> CVarRendering(
		TEXT("Atlas.Rendering"),
		1,
		TEXT("Enable Fluid Rendering \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
        
	TAutoConsoleVariable<int32> CVarSimulation(
		TEXT("Atlas.Simulation"),
		1,
		TEXT("Enable Fluid Simulation \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
}

FFluidExtention::FFluidExtention(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) 
{
	UE_LOG(LogTemp, Warning, TEXT("Atlas: Custom SceneViewExtension registered"));
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& InViewFamily)
{
    bIsReleasing = false;

    // Check for new volumes, that need particles
    UWorld* World = InViewFamily.Scene->GetWorld(); 
    if(World == nullptr) return;

    // Advance simulation
    if( CVarSimulation.GetValueOnRenderThread() == 1 && 
        (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE) && 
        !World->IsPaused()) 
    {
        TotalTime += World->GetDeltaSeconds();
    }
    
    for (TActorIterator<AActor> ActorItr(World); ActorItr; ++ActorItr)
    {
        AActor* Actor = *ActorItr;
        if (!Actor) continue;

        TArray<UFluidBoundingVolumeComponent*> FluidComponents;
        Actor->GetComponents<UFluidBoundingVolumeComponent>(FluidComponents);
        

        // Setup or Remove particle buffers
        for (UFluidBoundingVolumeComponent* FluidComp : FluidComponents)
        {
            if(!FluidComp) continue;
            switch (FluidComp->State)
            {
            case EAtlasVolumeState::RELEASE:
                ReleaseBufferComponents(FluidComp);
                bIsReleasing = true;
                break;

            case EAtlasVolumeState::GENERATE:
                GenerateBufferComponents(FluidComp);
                break;
                
            default:
                break;
            }
        }
    }
}

void FFluidExtention::PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InView) 
{  
    if(bIsReleasing) return;

    // Simulate
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(!ParticleBuffers->bInitialized) continue;

        // Note: Rendering assumes this is called here
        // NEED call this for ANY dispatch... 
        ParticleBuffers->Register(GraphBuilder);

        // Particle Simlation
        if(TotalTime > FixedTimeStep)
        {
            ParticleBuffers->SimulationSettings.DeltaTime = FixedTimeStep;
            TotalTime = 0.f;
        }
        else
        {
            // pause simulation if we're only rendering
            ParticleBuffers->SimulationSettings.DeltaTime = 0;
        }

        // Need to dispatch for rendering too...
        ParticleBuffers->DispatchFluidMath(GraphBuilder, GlobalShaderMap);
    }
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) 
{
    if(bIsReleasing) return;
    
    // Rendering
    if (CVarRendering.GetValueOnRenderThread() == 0) return;
    
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());
    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(!ParticleBuffers->bInitialized) continue;

        // Note: ParticleBuffers->Register() called in the simulation step already :)
        // Particle Rendering
        FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;
        ParticleBuffers->DispatchFluidRender(GraphBuilder, GlobalShaderMap, SceneColor, InView);
    }
}

void FFluidExtention::ReleaseBufferComponents(UFluidBoundingVolumeComponent* FluidComp)
{
    // Remove existing buffer
    TArray<USceneComponent*> Children;
    FluidComp->GetChildrenComponents(true, Children);
    for(USceneComponent* Child : Children)
    {
        UParticleBuffers* ParticleBuffers = nullptr;
        ParticleBuffers = dynamic_cast<UParticleBuffers*>(Child);
        if(ParticleBuffers)
        {
            FluidComp->Modify();
            ParticleBuffers->Modify();
            ParticleBuffers->UnregisterComponent(); // doesnt working, but doesnt break anything either
        }
    }

    FluidComp->State = EAtlasVolumeState::EMPTY;

    UE_LOG(LogTemp, Warning, TEXT("Atlas: Released Particle Buffers"));
}

void FFluidExtention::GenerateBufferComponents(UFluidBoundingVolumeComponent* FluidComp)
{
    const uint32 NumParticles = FluidComp->GetNumParticles();
    if(NumParticles > 5000 || NumParticles == 0)     
    {
        UE_LOG(LogTemp, Warning, TEXT("Atlas: Illegal NumParticles: %d"), NumParticles);
        return;
    }

    UParticleBuffers* NewParticleBuffers = NewObject<UParticleBuffers>(FluidComp);

    NewParticleBuffers->RegisterComponent();

    FluidComp->Modify();
    NewParticleBuffers->Modify();
    NewParticleBuffers->AttachToComponent(FluidComp, FAttachmentTransformRules::KeepRelativeTransform);

    // Allocate Particle Buffer
    NewParticleBuffers->Initialize(FluidComp);
    NewParticleBuffers->SimulationSettings.DeltaTime = FixedTimeStep;

    FluidComp->State = EAtlasVolumeState::SIMULATE;
    
    UE_LOG(LogTemp, Warning, TEXT("Atlas: Generated Particle Buffers"));
}

