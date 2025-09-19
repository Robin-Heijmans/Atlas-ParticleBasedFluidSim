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
		0,
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
    FluidMarch.Volume.BoundsPosition = FVector3f(0,0,0);
    FluidMarch.Volume.BoundsSize = FVector3f(1,1,1);
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) {
	// Dipatch Shader here
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;

    
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    // -- General Pipeline --
    // 1. Physics Simulation
    // 2. Generate Density Map / Render Prep
    // 3. Dispatch Fluid March / Rendering

    // Physics Simulation

    // Render Prep

    // Fluid March
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor);

}