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

#include "RenderGraphResources.h"
#include "Runtime/Engine/Classes/Engine/TextureRenderTarget2D.h"

#include "DispatchInterface.h"

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
	SHADER_PARAMETER(float, dummy)
	SHADER_PARAMETER(FVector3f, BoundsPosition)
	SHADER_PARAMETER(FVector3f, BoundsSize)
	SHADER_PARAMETER(float, dummy2)
	SHADER_PARAMETER(float, dummy3)
	SHADER_PARAMETER(FMatrix44f, View)
	SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D, RenderTarget)

END_SHADER_PARAMETER_STRUCT()

class SHADERINTERFACE_API FFluidMarchParams
{
	
	GENERATED_BODY()
public:
	int X;
	int Y;
	int Z;

	FVector3f EyePos;				//12 | 
	float dummy;					// 4 | 16
	// Assuming this is a rect
	FVector3f BoundsPosition;		//12 | 28
	FVector3f BoundsSize;			//12 | 40
	float dummy2, dummy3;			// 8 | 48

	FMatrix44f View;				//64 | 112
	
	FRenderTarget* RenderTarget;	// 8

	FFluidMarchParams(int x, int y, int z)
		: X(x)
		, Y(y)
		, Z(z)
	{
	}

	void DispatchRenderThread(FRDGBuilder& GraphBuilder) const;

};

// This is a static blueprint library that can be used to invoke our compute shader from blueprints.
UCLASS()
class SHADERINTERFACE_API UComputeShaderLibrary : public UObject
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	void ExecuteFluidMarch(FFluidMarchParams& DispatchParams);

private:
};
