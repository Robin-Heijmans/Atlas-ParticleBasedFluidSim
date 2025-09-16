// Example from: https://unreal.shadeup.dev/docs/compute (08.09.2025)

#pragma once

#include "CoreMinimal.h"
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

#include "Templates/UniquePtr.h"
#include "RenderGraphResources.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"

#include "ComputeLibrary.generated.h"


// SHADER_PARAMETER(uint32, MyUint32) // On the shader side: uint32 MyUint32;
// SHADER_PARAMETER(FVector3f, MyVector) // On the shader side: float3 MyVector;
// SHADER_PARAMETER_TEXTURE(Texture2D, MyTexture) // On the shader side: Texture2D<float4> MyTexture; (float4 should be whatever you expect each pixel in the texture to be, in this case float4(R,G,B,A) for 4 channels)
// SHADER_PARAMETER_SAMPLER(SamplerState, MyTextureSampler) // On the shader side: SamplerState MySampler; // CPP side: TStaticSamplerState<ESamplerFilter::SF_Bilinear>::GetRHI();
// SHADER_PARAMETER_ARRAY(float, MyFloatArray, [3]) // On the shader side: float MyFloatArray[3];
// SHADER_PARAMETER_UAV(RWTexture2D<FVector4f>, MyTextureUAV) // On the shader side: RWTexture2D<float4> MyTextureUAV;
// SHADER_PARAMETER_UAV(RWStructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWStructuredBuffer<FMyCustomStruct> MyCustomStructs;
// SHADER_PARAMETER_UAV(RWBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: RWBuffer<FMyCustomStruct> MyCustomStructs;
// SHADER_PARAMETER_SRV(StructuredBuffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: StructuredBuffer<FMyCustomStruct> MyCustomStructs;
// SHADER_PARAMETER_SRV(Buffer<FMyCustomStruct>, MyCustomStructs) // On the shader side: Buffer<FMyCustomStruct> MyCustomStructs;
// SHADER_PARAMETER_SRV(Texture2D<FVector4f>, MyReadOnlyTexture) // On the shader side: Texture2D<float4> MyReadOnlyTexture;
// SHADER_PARAMETER_STRUCT_REF(FMyCustomStruct, MyCustomStruct)


BEGIN_SHADER_PARAMETER_STRUCT(FFluidDispatchParams, )
	SHADER_PARAMETER(FVector3f, EyePos)
	SHADER_PARAMETER(FVector3f, BoundsPosition)
	SHADER_PARAMETER(FVector3f, BoundsSize)
	SHADER_PARAMETER(FMatrix44f, View)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, RenderTarget)
	
END_SHADER_PARAMETER_STRUCT()

USTRUCT(BlueprintType)
struct SHADERINTERFACE_API FFluidMarchParams
{	
	GENERATED_BODY()
	
public:
	int X = 1;
	int Y = 1;
	int Z = 1;

	UPROPERTY(EditAnywhere)
	FVector3f EyePos = FVector3f(1.f, 1.f, -1.f);				//12 | 
	//float dummy = -1.f;											// 4 | 16
	// Assuming this is a rect
	UPROPERTY(EditAnywhere)
	FVector3f BoundsPosition = FVector3f(0.f, 0.f, 0.f);;		//12 | 28
	UPROPERTY(EditAnywhere)
	FVector3f BoundsSize = FVector3f(1.f, 1.f, 1.f);			//12 | 40
	//float dummy2 = -1.f, dummy3 = -1.f;							// 8 | 48

	UPROPERTY(EditAnywhere)
	FMatrix44f View = FMatrix44f();								//64 | 112
	
	FRenderTarget* RenderTarget = nullptr;						// 8


	FFluidMarchParams() = default;
	FFluidMarchParams(int x, int y, int z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}
	
	void Dispatch(FRDGBuilder& GraphBuilder);
};

UCLASS()
class SHADERINTERFACE_API UComputeLibrary : public UActorComponent
{
GENERATED_BODY()
public:

	template<typename TShaderParams>
	static void ExecuteShader(TShaderParams& ParamStruct)
	{
		if (IsInRenderingThread()) 
		{
			DispatchRenderThread(GetImmediateCommandList_ForRenderCommand(), ParamStruct);
		}
		else
		{
    		UE_LOG(LogTemp, Warning, TEXT("Not in rendering thread"));
			DispatchRenderThread_Game(ParamStruct);
		}
	}

private:

	template<typename TShaderParams>
	static void DispatchRenderThread(FRHICommandListImmediate& RHICmdList, TShaderParams& ParamStruct)
	{
		FRDGBuilder GraphBuilder(RHICmdList);
		ParamStruct.Dispatch(GraphBuilder);
	}

	template<typename TShaderParams>
	static void DispatchRenderThread_Game(TShaderParams& ParamStruct)
	{
		ENQUEUE_RENDER_COMMAND(SceneDrawCompletion)(
		[&ParamStruct](FRHICommandListImmediate& RHICmdList)
		{
			FRDGBuilder GraphBuilder(GetImmediateCommandList_ForRenderCommand());
			ParamStruct.Dispatch(GraphBuilder);
		});
	}
};

