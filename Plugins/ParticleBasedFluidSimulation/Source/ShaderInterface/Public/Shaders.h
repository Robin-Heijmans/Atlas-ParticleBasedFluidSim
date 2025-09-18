#pragma once
#include "ComputeLibrary.h"

#include "Shaders.generated.h"

/// look at these comments for naming conventions pls

// ---------
// This file contains:
//
// DispatchParams for thread groups and C++ side implementation for ShaderParameters
//
// Start with ShaderParameters, for HLSL declerations for shader structss
//
// GlobalShader implementation 
// ---------


// ---------
// USTRUCT() struct F*ShaderName*DispatchParams
// int x,y,z for numthreads
// members... (dont forget to add UPROPERT())
// needs default constructor
//  
// IMPORTANT add function decleration:
// void Dispatch(FRDGBuilder& GraphBuilder); -> defined in Shaders.cpp
// ---------

// Cannot be in a namespace, generated_body() is being skipped (???)
USTRUCT(BlueprintType)
struct SHADERINTERFACE_API FFluidMarchDispatchParams
{	
    GENERATED_BODY()
public:
    int X = 1;
    int Y = 1;
    int Z = 1;

    FVector3f EyePos = FVector3f(1.f, 1.f, -1.f);				//12 
    FVector3f BoundsPosition = FVector3f(0.f, 0.f, 0.f);;		//12
    FVector3f BoundsSize = FVector3f(1.f, 1.f, 1.f);			//12 

    FMatrix44f View = FMatrix44f();								//64

    FRenderTarget* RenderTarget = nullptr;						// 8


    FFluidMarchDispatchParams() = default;
    FFluidMarchDispatchParams(int x, int y, int z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }
    void Dispatch(FRDGBuilder& GraphBuilder);
};

namespace Shaders
{
    namespace ShaderParameters
    {
        
        // ---------
        // Begin(F*ShaderName*Params later used in global shader for using FParameters = F*ShaderName*Params)
        // Shader_Params() ... 
        // End()
        // ---------

        BEGIN_UNIFORM_BUFFER_STRUCT(FFluidUB, )
            SHADER_PARAMETER(FVector3f, BoundsPosition)
            SHADER_PARAMETER(FVector3f, BoundsSize)
        END_UNIFORM_BUFFER_STRUCT()

        BEGIN_SHADER_PARAMETER_STRUCT(FFluidMarchParams, )
            SHADER_PARAMETER_STRUCT_REF(ShaderParameters::FFluidUB, Fluid)
            SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    		SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColor)
            SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, Target)

        END_SHADER_PARAMETER_STRUCT()
    } // namespace ShaderParameterss


    /// Shader Classes

    // ---------
    // F*ShaderName*Shader
    // ... using FParameters = F*ShaderName*ShaderParams
    // ---------

    // This class carries our parameter declarations and acts as the bridge between cpp and HLSL.
    class FFluidMarchShader : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMarchShader);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMarchShader, FGlobalShader);

    	using FParameters = ShaderParameters::FFluidMarchParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

} // namespace Shaders