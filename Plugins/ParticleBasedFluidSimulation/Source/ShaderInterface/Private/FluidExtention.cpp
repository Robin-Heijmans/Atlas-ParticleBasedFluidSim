#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneView.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PostProcess/PostProcessInputs.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UnifiedBuffer.h"

#include "ComputeLibrary.h"

namespace 
{
	TAutoConsoleVariable<int32> CVarShaderOn(
		TEXT("r.Fluid"),
		0,
		TEXT("Enable Fluid Rendering \n")
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

        FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("DensityMap"))
                .SetExtent(512, 512)
                .SetDepth(512)
                .SetFormat(PF_A32B32G32R32F)
                .SetFlags(ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource)
                .SetInitialState(ERHIAccess::SRVCompute);

        DensityMap = RHICreateTexture(Desc);
        
        const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluidShader_DensityMap")));
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef);

        GraphBuilder.Execute();
    });
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) 
{

}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) 
{
	// Dipatch Shader here
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;

    // Get ShaderMap
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    // Update Uniform Buffers
    // Particles
    const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluidShader_DensityMap")));

    // -- General Pipeline --
    // 1. Physics Simulation
    // 2. Generate Density Map / Render Prep
    // 3. Dispatch Fluid March / Rendering

    // Physics Simulation
    //ParticleSimulation.Dispatch(GraphBuilder, GlobalShaderMap, Particles);
    //FluidMath.Positions

    //FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, MathParams);
    //FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, MathParams);
    //FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, MathParams);
    //FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, MathParams);
    //FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, MathParams);
    //FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, MathParams);

    // Render Prep
    //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef);

    // Fluid March
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor, FluidVolume, DensityMapRef);

}