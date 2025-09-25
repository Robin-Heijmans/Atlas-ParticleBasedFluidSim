#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "PostProcess/PostProcessInputs.h"
#include "Misc/Optional.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"	
#include "RHICommandList.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UnifiedBuffer.h"

#include "FluidBoundingVolume.h"
#include "ParticleLibrary.h"

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
        
        const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluid DensityMap")));
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, PositionsRef);

        //GraphBuilder.QueueBufferExtraction(ParticlePositionsRef, &PooledPositions);
        GraphBuilder.Execute();
    });
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) 
{
    if (CVarSimulation.GetValueOnRenderThread() == 0) return; 

    UWorld* World = GEngine->GetWorld();
    if(World == nullptr) return;

    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(ParticleBuffers->GetWorld() != World) continue;
        ENQUEUE_RENDER_COMMAND(GenDensityMap)(
        [this, ParticleBuffers](FRHICommandListImmediate& RHICmdList) {
            FRDGBuilder GraphBuilder(RHICmdList);
                
            FFluidMathParams FluidMath = ParticleBuffers->GetParticleParameters(GraphBuilder);

            FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::SortAndCalculateOffsets(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, FluidMath);
            FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, FluidMath);

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

    // Update Uniform Buffers
    // Particles

    // -- General Pipeline --
    // 1. Physics Simulation
    // 2. Generate Density Map / Render Prep
    // 3. Dispatch Fluid March / Rendering

    // Physics Simulation
    //ParticleSimulation.Dispatch(GraphBuilder, GlobalShaderMap, Particles);

    // Render Prep
    const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluid DensityMap")));
    //const FRDGBufferRef PositionsRef = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Positions, TEXT("TanFluid Positions"));
        
    //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, PositionsRef);

    // Fluid March
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor, FluidVolume, DensityMapRef);

}