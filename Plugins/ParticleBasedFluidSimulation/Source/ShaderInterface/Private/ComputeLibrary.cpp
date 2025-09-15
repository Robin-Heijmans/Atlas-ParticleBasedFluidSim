#include "ComputeLibrary.h"

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

// Probs a good idea to keep it this size, (maybe look into 64x8x1)
#define NUM_THREADS_ComputeShader_X 32
#define NUM_THREADS_ComputeShader_Y 32
#define NUM_THREADS_ComputeShader_Z 1

DECLARE_STATS_GROUP(TEXT("TanFluidMarch"), STATGROUP_ComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("TanFluidMarch Execute"), STAT_ComputeShader_Execute, STATGROUP_ComputeShader);


// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class SHADERINTERFACE_API FFluidMarchShader : public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FFluidMarchShader);
	SHADER_USE_PARAMETER_STRUCT(FFluidMarchShader, FGlobalShader);
	using FParameters = FFluidDispatchParams;
	
	class FFluidMarchShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FFluidMarchShader_Perm_TEST
	>;

public:
	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		const FPermutationDomain PermutationVector(Parameters.PermutationId);
		
		return true;
	}

	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		const FPermutationDomain PermutationVector(Parameters.PermutationId);

		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_ComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_ComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_ComputeShader_Z);
	}
private:
};

void FFluidMarchParams::Dispatch(FRDGBuilder& GraphBuilder)  
{

    UE_LOG(LogTemp, Warning, TEXT("Executing TanFluid"));

	SCOPE_CYCLE_COUNTER(STAT_ComputeShader_Execute);
	DECLARE_GPU_STAT(ComputeShader)
	RDG_EVENT_SCOPE(GraphBuilder, "ComputeShader");
	RDG_GPU_STAT_SCOPE(GraphBuilder, ComputeShader);

	typename FFluidMarchShader::FPermutationDomain PermutationVector;
	TShaderMapRef<FFluidMarchShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
	bool bIsShaderValid = ComputeShader.IsValid();
	if (bIsShaderValid) 
	{
		FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FFluidMarchShader::FParameters>();

		FRDGTextureDesc Desc = FRDGTextureDesc::Create2D(RenderTarget->GetSizeXY(), 
		PF_B8G8R8A8, 
		FClearValueBinding::White, 
		TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV);

		FRDGTextureRef TmpTexture = GraphBuilder.CreateTexture(Desc, TEXT("TanComputeShader_TempTexture"));
		FRDGTextureRef TargetTexture = RegisterExternalTexture(GraphBuilder, RenderTarget->GetRenderTargetTexture(), TEXT("TanComputeShader_RT"));
		PassParameters->RenderTarget = GraphBuilder.CreateUAV(TmpTexture);
		PassParameters->EyePos = EyePos;
		auto GroupCount = FComputeShaderUtils::GetGroupCount(FIntVector(X, Y, Z), FComputeShaderUtils::kGolden2DGroupSize);
		GraphBuilder.AddPass(
			RDG_EVENT_NAME("ExecuteComputeShader"),
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
		#if WITH_EDITOR
			GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
		#endif
		// We exit here as we don't want to crash the game if the shader is not found or has an error.
	}
	GraphBuilder.Execute();
}

// This will tell the engine to create the shader and where the shader entry point is.
//                      ShaderType      ShaderPath                  Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FFluidMarchShader, "/Shaders/Compute/FluidMarch.usf", "Compute", SF_Compute);