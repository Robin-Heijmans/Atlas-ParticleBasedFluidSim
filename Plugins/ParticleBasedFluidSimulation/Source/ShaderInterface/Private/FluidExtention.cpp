#include "FluidExtention.h"

#include "EngineUtils.h"
#include "PostProcess/PostProcessInputs.h"
#include "Misc/Optional.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "SceneView.h"
#include "Engine/World.h"

#include "GenericPlatform/GenericPlatformMisc.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialRenderProxy.h"
#include "MeshPassProcessor.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "MeshMaterialShader.h"
#include "ShaderParameterUtils.h"
#include "RHIStaticStates.h"
#include "Shader.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "UniformBuffer.h"
#include "RHICommandList.h"
#include "ShaderCompilerCore.h"
#include "EngineDefines.h"
#include "RendererInterface.h"
#include "RenderResource.h"
#include "RenderGraphResources.h"
#include "Components/ActorComponent.h"

#include "ComputeLibrary.h"

FFluidExtention::FFluidExtention(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) {
	UE_LOG(LogTemp, Log, TEXT("Fluid: Custom SceneViewExtension registered"));
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) {
	/* Get the world from the scene */
	UWorld* World = ViewFamily.Scene->GetWorld();
	if (World == nullptr) return;

    AActor* Actor = UGameplayStatics::GetActorOfClass(ViewFamily.Scene->GetWorld(), AActor::StaticClass());
    RenderTest = UKismetRenderingLibrary::CreateRenderTarget2D(Actor, 1920, 1080, RTF_RGBA8);

    // Default Params for now
    FluidMarch.EyePos = FVector3f(1,1,1);
    FluidMarch.BoundsPosition = FVector3f(0,0,0);
    FluidMarch.BoundsSize = FVector3f(1,1,1);
    FluidMarch.View = FMatrix44f();
    
    FluidMarch.RenderTarget = RenderTest->GameThread_GetRenderTargetResource();
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) {
	// Dipatch Shader here

DECLARE_GPU_STAT(ComputeShader)
    RDG_EVENT_SCOPE(GraphBuilder, "TanComputeShader");
    RDG_GPU_STAT_SCOPE(GraphBuilder, ComputeShader);

    TShaderMapRef<Shaders::FFluidMarchShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    bool bIsShaderValid = ComputeShader.IsValid();
    if (bIsShaderValid) 
    {

        Shaders::FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMarchShader::FParameters>();

        FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(FluidMarch.RenderTarget->GetSizeXY(), 
        PF_B8G8R8A8, 
        FClearValueBinding::White, 
        TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV);

        FRDGTextureRef TmpTexture = GraphBuilder.CreateTexture(Desc, TEXT("TanComputeShader_TempTexture"));
        FRDGTextureRef TargetTexture = RegisterExternalTexture(GraphBuilder, FluidMarch.RenderTarget->GetRenderTargetTexture(), TEXT("TanComputeShader_Output"));

        PassParameters->RenderTarget = GraphBuilder.CreateUAV(TmpTexture);
        PassParameters->EyePos = FluidMarch.EyePos;
        PassParameters->BoundsPosition = FluidMarch.BoundsPosition;
        PassParameters->BoundsSize = FluidMarch.BoundsSize;
        PassParameters->View = FluidMarch.View;

        auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(FluidMarch.X, FluidMarch.Y, FluidMarch.Z), FComputeShaderUtils::kGolden2DGroupSize);
        UE_LOG(LogTemp, Warning, TEXT("Adding DispatchPass TanFluid"));
        GraphBuilder.AddPass(
            RDG_EVENT_NAME("Execute TanComputeShader"),
            PassParameters,
            ERDGPassFlags::AsyncCompute,
            [&PassParameters, ComputeShader, GroupCount](FRHIComputeCommandList& RHICmdList)
        {
            FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, *PassParameters, GroupCount);
        });


        // The copy will fail if we don't have matching formats, let's check and make sure we do.
        if (TargetTexture->Desc.Format == PF_B8G8R8A8) 
        {
            AddCopyTexturePass(GraphBuilder, TmpTexture, TargetTexture, FRHICopyTextureInfo());
        } 
        else 
        {
            #if WITH_EDITOR
                GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The provided render target has an incompatible format (Please change the RT format to: RGBA8).")));
            #endif
        }
    
    } 
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("The compute shader has a problem."));
    }
    UE_LOG(LogTemp, Warning, TEXT("Dispatch you fucking bitch"));
    GraphBuilder.Execute();


    
    //if(FluidMarch.RenderTarget != nullptr)
    //{
    //    UComputeLibrary::ExecuteShader(FluidMarch, GraphBuilder);
    //}
}