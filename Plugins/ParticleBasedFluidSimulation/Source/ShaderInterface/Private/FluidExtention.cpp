#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "PostProcess/PostProcessInputs.h"
#include "Misc/Optional.h"
#include "GameFramework/Actor.h"
#include "RHICommandList.h"

#include "Components/BoxComponent.h"
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

    // Default Params for now
    FluidVolume.BoundsPosition = FVector3f(-288.779695,8.043881,122.825553);
    FluidVolume.BoundsSize = FVector3f(56,56,56);

    ENQUEUE_RENDER_COMMAND(GenDensityMap)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        //Density Map UwU
        FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("DensityMap"))
                .SetExtent(128, 128)
                .SetDepth(128)
                .SetFormat(PF_A32B32G32R32F)
                .SetFlags(ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource)
                .SetInitialState(ERHIAccess::SRVCompute);

        DensityMap = RHICreateTexture(Desc);
        
        const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("Atlas DensityMap")));
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, PositionsRef);

        //GraphBuilder.QueueBufferExtraction(ParticlePositionsRef, &PooledPositions);
        GraphBuilder.Execute();
    });
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) 
{
    if (CVarSimulation.GetValueOnRenderThread() == 0) return; 

    // Check for new volumes, that need particles
    UWorld* World = ViewFamily.Scene->GetWorld();
    if(World == nullptr) return;
    bool Wait = false;
    for (TActorIterator<AFluidBoundingVolume> FluidVolumes(World); FluidVolumes; ++FluidVolumes)
    {
        if(!FluidVolumes->HasParticles)
        {   
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

            FFluidVolumeLocal VolumeBounds; 
            VolumeBounds.MaxBounds = FVector3f(FluidVolumes->Bounds->GetScaledBoxExtent());
            VolumeBounds.MinBounds = FVector3f(0,0,0);

            ParticleBuffers->Initialize(NumParticles, VolumeBounds);
            FluidVolumes->HasParticles = true;
            return;
        }
    }

    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(!ParticleBuffers->bInitialized) continue;
        
        ENQUEUE_RENDER_COMMAND(ParticleSimulation)(
        [this, ParticleBuffers](FRHICommandListImmediate& RHICmdList) {
            FRDGBuilder GraphBuilder(RHICmdList);
            
            ParticleBuffers->Register(GraphBuilder);
            
            FFluidMathParams FluidMath = ParticleBuffers->GetParticleParameters(GraphBuilder);

            FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::SortAndCalculateOffsets(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, FluidMath);

            // Render Prep
            FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("Atlas DensityMap")));
            FRDGBufferSRVRef PositionsRef = ParticleBuffers->GetRenderPrepParameters(GraphBuilder);

            FRenderPrepParams RenderPrepParams;
            RenderPrepParams.DensityMap = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DensityMapRef));
            RenderPrepParams.Positions = PositionsRef;

            RenderPrepParams.DensityMapSize = 128; // PLS PUT ME OUT OF MY MISERY
            RenderPrepParams.NumParticles = ParticleBuffers->NumParticles;

            RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, RenderPrepParams);
                
            GraphBuilder.Execute();
        });

    }
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) 
{

	// Dipatch Shader here
    if (CVarRendering.GetValueOnRenderThread() == 0) return; 

    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;

    // Get ShaderMap
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    // Dispatch Fluid March / Rendering
    const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("Atlas DensityMap")));
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor, FluidVolume, DensityMapRef);

}