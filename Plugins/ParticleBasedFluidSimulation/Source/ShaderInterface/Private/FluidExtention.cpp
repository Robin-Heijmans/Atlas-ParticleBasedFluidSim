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
    FluidMarch.EyePos = FVector3f(1,1,1);
    FluidMarch.BoundsPosition = FVector3f(0,0,0);
    FluidMarch.BoundsSize = FVector3f(1,1,1);
    FluidMarch.View = FMatrix44f();
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) {
	// Dipatch Shader here
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;
	const FIntPoint ViewSize = SceneColor->Desc.Extent;

    RDG_EVENT_SCOPE(GraphBuilder, "TanComputeShader");

    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    Shaders::FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMarchShader::FParameters>();

    FRDGTextureDesc OutputDesc {};
    OutputDesc = SceneColor->Desc;
    OutputDesc.Reset();
    OutputDesc.Flags |= TexCreate_UAV;
    OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
    const FLinearColor ClearColor(0., 0., 0., 0.);
    OutputDesc.ClearValue = FClearValueBinding(ClearColor);

    const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("TanFluidShader_Output"));

    FluidData.BoundsPosition = FVector3f(0,0,0);
    FluidData.BoundsSize = FVector3f(1,1,1);
    
    PassParameters->Target = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
    PassParameters->Fluid = TUniformBufferRef<Shaders::ShaderParameters::FFluidUB>::CreateUniformBufferImmediate(FluidData, EUniformBufferUsage::UniformBuffer_SingleFrame);
    PassParameters->SceneColor = SceneColor;
    PassParameters->View = InView.ViewUniformBuffer;

    const FIntVector DispatchCount = FComputeShaderUtils::GetGroupCount(ViewSize, FComputeShaderUtils::kGolden2DGroupSize);
    
    TShaderMapRef<Shaders::FFluidMarchShader> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute TanComputeShader %dx%d", ViewSize.X, ViewSize.Y),
        ComputeShader,
        PassParameters,
        DispatchCount);

    AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor);

    //GraphBuilder.Execute();
}