#include "Shaders.h"

#include "PixelShaderUtils.h"
#include "MeshPassProcessor.inl"
#include "StaticMeshResources.h"
#include "DynamicMeshBuilder.h"
#include "RenderGraphResources.h"
#include "GlobalShader.h"
#include "UnifiedBuffer.h"
#include "CanvasTypes.h"
#include "MeshDrawShaderBindings.h"
#include "RHIGPUReadback.h"
#include "MeshPassUtils.h"
#include "MaterialShader.h"



// ---------
// This file contains
// IMPLEMENT_GLOBAL_SHADER()/BUFFER() implementations for shader classes from Shaders.h
//
// Dispatch functions from DispatchShaderParams from Shader.h
// (these functions are being called on by the compute library)
// ---------

namespace Shaders
{

    // Implementations ... 
    
    //                      ShaderType      ShaderPath                  Shader function name    Type
    IMPLEMENT_GLOBAL_SHADER(FFluidMarchShader, "/Shaders/Compute/FluidMarch.usf", "Compute", SF_Compute);

    // ... add new implemenations here
    
}

// Dispatch Functions ...
void FFluidMarchDispatchParams::Dispatch(FRDGBuilder& GraphBuilder)  
{
    TShaderMapRef<Shaders::FFluidMarchShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

    bool bIsShaderValid = ComputeShader.IsValid();
    if (bIsShaderValid) 
    {
        DECLARE_GPU_STAT(ComputeShader)
        //RDG_EVENT_SCOPE(GraphBuilder, "TanComputeShader");
        RDG_GPU_STAT_SCOPE(GraphBuilder, ComputeShader);

        Shaders::FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMarchShader::FParameters>();

        FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(RenderTarget->GetSizeXY(), 
        PF_B8G8R8A8, 
        FClearValueBinding::White, 
        TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV);

        FRDGTextureRef TmpTexture = GraphBuilder.CreateTexture(Desc, TEXT("TanComputeShader_TempTexture"));
        FRDGTextureRef TargetTexture = RegisterExternalTexture(GraphBuilder, RenderTarget->GetRenderTargetTexture(), TEXT("TanComputeShader_Output"));

        PassParameters->RenderTarget = GraphBuilder.CreateUAV(TmpTexture);
        PassParameters->EyePos = EyePos;
        PassParameters->BoundsPosition = BoundsPosition;
        PassParameters->BoundsSize = BoundsSize;
        PassParameters->View = View;

        auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(X, Y, Z), FComputeShaderUtils::kGolden2DGroupSize);
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
    
        GraphBuilder.Execute();
    } 
    else 
    {
        UE_LOG(LogTemp, Warning, TEXT("The compute shader has a problem."));
    }
}