#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SceneView.h"
#include "Engine/TextureRenderTarget2D.h"
#include "PostProcess/PostProcessInputs.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"

#include "ComputeLibrary.h"

namespace {
	TAutoConsoleVariable<int32> CVarShaderOn(
		TEXT("r.Fluid"),
		1,
		TEXT("Enable Fluid Rendering \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
}

FFluidExtention::FFluidExtention(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {
	UE_LOG(LogTemp, Log, TEXT("Fluid: Custom SceneViewExtension registered"));
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) {

    // Default Params for now
    FluidVolume.BoundsPosition = FVector3f(0.5,0,0.5);
    FluidVolume.BoundsSize = FVector3f(1,1,1);
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) {
	// Dipatch Shader here
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;

    // Get ShaderMap
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    // Update Uniform Buffers
    
    
    // -- General Pipeline --
    // 1. Physics Simulation
    // 2. Generate Density Map / Render Prep
    // 3. Dispatch Fluid March / Rendering

    // Physics Simulation
    TArray<FVector3f> Positions; 
    Positions.Init(FVector3f(1,-1,1), 16);

    ParticleSimulation.CreateBuffers(GraphBuilder, Positions);
    ParticleSimulation.Dispatch(GraphBuilder, GlobalShaderMap);

    // Render Prep
    //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, FluidParticles);

    // Fluid March
    //FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor, FluidVolume);
    //GraphBuilder.Execute();
}