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

DECLARE_STATS_GROUP(TEXT("ComputeShader"), STATGROUP_ComputeShader, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("ComputeShader Execute"), STAT_ComputeShader_Execute, STATGROUP_ComputeShader);

// This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
class SHADERINTERFACE_API FComputeShader: public FGlobalShader
{
public:
	
	DECLARE_GLOBAL_SHADER(FComputeShader);
	SHADER_USE_PARAMETER_STRUCT(FComputeShader, FGlobalShader);
	using FParameters = FFluidDispatchParams;
	
	class FComputeShader_Perm_TEST : SHADER_PERMUTATION_INT("TEST", 1);
	using FPermutationDomain = TShaderPermutationDomain<
		FComputeShader_Perm_TEST
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

		/*
		* Here you define constants that can be used statically in the shader code.
		* :
		*/
		// OutEnvironment.SetDefine(TEXT("MY_CUSTOM_CONST"), TEXT("1"));

		/*
		* These defines are used in the thread count section of our shader
		*/
		OutEnvironment.SetDefine(TEXT("THREADS_X"), NUM_THREADS_ComputeShader_X);
		OutEnvironment.SetDefine(TEXT("THREADS_Y"), NUM_THREADS_ComputeShader_Y);
		OutEnvironment.SetDefine(TEXT("THREADS_Z"), NUM_THREADS_ComputeShader_Z);

		// This shader must support typed UAV load and we are testing if it is supported at runtime using RHIIsTypedUAVLoadSupported
		//OutEnvironment.CompilerFlags.Add(CFLAG_AllowTypedUAVLoads);

		// FForwardLightingParameters::ModifyCompilationEnvironment(Parameters.Platform, OutEnvironment);
	}
private:
};

void FFluidMarchParams::DispatchRenderThread(FRDGBuilder& GraphBuilder) const
	{
		SCOPE_CYCLE_COUNTER(STAT_ComputeShader_Execute);
		DECLARE_GPU_STAT(ComputeShader)
		RDG_EVENT_SCOPE(GraphBuilder, "TanComputeShader");
		RDG_GPU_STAT_SCOPE(GraphBuilder, ComputeShader);
		
		typename FComputeShader::FPermutationDomain PermutationVector;
		
		// Add any static permutation options here
		// PermutationVector.Set<FComputeShader::FMyPermutationName>(12345);

		TShaderMapRef<FComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel), PermutationVector);
		

		bool bIsShaderValid = ComputeShader.IsValid();

		if (bIsShaderValid) {
			FComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FComputeShader::FParameters>();

			
			FRDGTextureDesc Desc(FRDGTextureDesc::Create2D(RenderTarget->GetSizeXY(), PF_B8G8R8A8, FClearValueBinding::White, TexCreate_RenderTargetable | TexCreate_ShaderResource | TexCreate_UAV));
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
			if (TargetTexture->Desc.Format == PF_B8G8R8A8) {
				AddCopyTexturePass(GraphBuilder, TmpTexture, TargetTexture, FRHICopyTextureInfo());
			} else {
				#if WITH_EDITOR
					GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The provided render target has an incompatible format (Please change the RT format to: RGBA8).")));
				#endif
			}
			
		} else {
			#if WITH_EDITOR
				GEngine->AddOnScreenDebugMessage((uint64)42145125184, 6.f, FColor::Red, FString(TEXT("The compute shader has a problem.")));
			#endif

			// We exit here as we don't want to crash the game if the shader is not found or has an error.
			
		}
	GraphBuilder.Execute();
}


void UComputeShaderLibrary::ExecuteFluidMarch(FFluidMarchParams& DispatchParams)
{
	FComputeShaderInterface<FFluidMarchParams>::Dispatch(DispatchParams);
}


// This will tell the engine to create the shader and where the shader entry point is.
//                      ShaderType      ShaderPath                  Shader function name    Type
IMPLEMENT_GLOBAL_SHADER(FComputeShader, "/Shaders/FluidsMarch.usf", "Compute", SF_Compute);