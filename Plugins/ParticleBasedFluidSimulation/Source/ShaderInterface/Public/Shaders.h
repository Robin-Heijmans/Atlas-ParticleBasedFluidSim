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
// Begin(F*ShaderName*Params later used in global shader for using FParameters = F*ShaderName*Params)
// Shader_Params() ... 
// End()
// ---------

// Fluid Volume UB
BEGIN_UNIFORM_BUFFER_STRUCT(FFluidVolume, )
    SHADER_PARAMETER(FVector3f, BoundsPosition)
    SHADER_PARAMETER(FVector3f, BoundsSize)
END_UNIFORM_BUFFER_STRUCT()
    
// FluidMarch
BEGIN_SHADER_PARAMETER_STRUCT(FFluidMarchParams, )
    SHADER_PARAMETER_STRUCT_REF(FFluidVolume, Volume)
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColor)
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, Target)

END_SHADER_PARAMETER_STRUCT()



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

// Fluid March // Actually rendering to screen
USTRUCT(BlueprintType)
struct SHADERINTERFACE_API FFluidMarchDispatchParams
{	
    GENERATED_BODY()
public:
    int X = 1;
    int Y = 1;
    int Z = 1;

    FFluidVolume Volume;

    FFluidMarchDispatchParams() = default;
    FFluidMarchDispatchParams(int x, int y, int z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }
    void Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, const FSceneView& InView, FRDGTexture* SceneColor);
};

// GenerateDensityMap
USTRUCT(BlueprintType)
struct SHADERINTERFACE_API FGenerateDensityMapDispatchParams
{	
GENERATED_BODY()
    public:
    int X = 1;
    int Y = 1;
    int Z = 1;

    // 3d Texture for read/write ops

    FGenerateDensityMapDispatchParams() = default;
    FGenerateDensityMapDispatchParams(int x, int y, int z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    void Dispatch(FRDGBuilder& GraphBuilder);
};

// SimulateParticles
USTRUCT(BlueprintType)
struct SHADERINTERFACE_API FSimulateParticlesDispatchParams
{	
GENERATED_BODY()
    public:
    int X = 1;
    int Y = 1;
    int Z = 1;

    // Particle data, buffers for position, velocity, etc.

    FSimulateParticlesDispatchParams() = default;
    FSimulateParticlesDispatchParams(int x, int y, int z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    void Dispatch(FRDGBuilder& GraphBuilder);
};


namespace Shaders
{
    /// Shader Classes

    // ---------
    // F*ShaderName*Shader
    // ... using FParameters = F*ShaderName*ShaderParams
    // ---------

    class FParticleSimulationShader : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FParticleSimulationShader);
    	SHADER_USE_PARAMETER_STRUCT(FParticleSimulationShader, FGlobalShader);

    	using FParameters = FSimulateParticlesDispatchParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FRenderPrepShader : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FRenderPrepShader);
    	SHADER_USE_PARAMETER_STRUCT(FRenderPrepShader, FGlobalShader);

    	using FParameters = FGenerateDensityMapDispatchParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMarchShader : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMarchShader);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMarchShader, FGlobalShader);

    	using FParameters = FFluidMarchParams;

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