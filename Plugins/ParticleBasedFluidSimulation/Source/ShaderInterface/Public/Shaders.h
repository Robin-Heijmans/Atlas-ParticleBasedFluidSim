#pragma once

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

// -- Common.ush --
// These should be owned by the scene view extention, not the DispatchParams themselves as they are shared across shaders
// Fluid Volume UB
BEGIN_UNIFORM_BUFFER_STRUCT(FFluidVolume, )
    SHADER_PARAMETER(FVector3f, BoundsPosition)
    SHADER_PARAMETER(FVector3f, BoundsSize)
END_UNIFORM_BUFFER_STRUCT()

BEGIN_UNIFORM_BUFFER_STRUCT(FFluidVolumeLocal, )
    SHADER_PARAMETER(FVector3f, MinBounds)
    SHADER_PARAMETER(FVector3f, MaxBounds)
END_UNIFORM_BUFFER_STRUCT()

BEGIN_UNIFORM_BUFFER_STRUCT(FFluidEnvironment, )

    SHADER_PARAMETER(FMatrix44f, CubeLocalToWorld)
    SHADER_PARAMETER(FMatrix44f, CubeWorldToLocal)
    SHADER_PARAMETER(FVector3f, ExtinctionCoeff)
    SHADER_PARAMETER(float, MarchStepSize)
    SHADER_PARAMETER(float, LightStepSize)
    SHADER_PARAMETER(float, DensityStepSize)
    SHADER_PARAMETER(float, DensityMultiplier)
    SHADER_PARAMETER(float, indexOfRefraction)
    SHADER_PARAMETER(uint32, NumRefractions)

END_UNIFORM_BUFFER_STRUCT()

// -- .usf files --
// FluidMath
BEGIN_SHADER_PARAMETER_STRUCT(FFluidMathParams, )
    SHADER_PARAMETER_STRUCT_REF(FFluidVolumeLocal, FluidBounds)

    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, Positions)
    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, PredictedPositions)
    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float3>, Velocities)
    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<float>, Densities)
    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint3>, SpatialIndices) //uint3(index, hash, key)
    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, SpatialOffsets)
    
    SHADER_PARAMETER(float, PressureAmplifier)
    SHADER_PARAMETER(float, TargetDensity)
    SHADER_PARAMETER(float, CollisionDampening)
    SHADER_PARAMETER(float, SmoothingRadius)
    SHADER_PARAMETER(float, ViscosityStrength)
    SHADER_PARAMETER(float, DeltaTime)
    SHADER_PARAMETER(FVector3f, Gravity)
    SHADER_PARAMETER(uint32, NumParticles)

    SHADER_PARAMETER(FMatrix44f, OtherLocalTransform)
    SHADER_PARAMETER(FMatrix44f, OtherLocalTransformInverse)
    SHADER_PARAMETER(FVector3f, OtherLocalExtent)
    SHADER_PARAMETER(FVector3f, LocalScale)
    SHADER_PARAMETER(FVector3f, LocalCenter)

END_SHADER_PARAMETER_STRUCT()

BEGIN_SHADER_PARAMETER_STRUCT(FFluidGPUSortParams, )

    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint3>, Entries) //uint3(index, hash, key)
    SHADER_PARAMETER_RDG_BUFFER_UAV(RWStructuredBuffer<uint>, Offsets)

    SHADER_PARAMETER(int, numEntries)
    SHADER_PARAMETER(int, groupWidth)
    SHADER_PARAMETER(int, groupHeight)
    SHADER_PARAMETER(int, stepIndex)

END_SHADER_PARAMETER_STRUCT()

// -- .usf files --

// RenderPrep
BEGIN_SHADER_PARAMETER_STRUCT(FRenderPrepParams, )
    SHADER_PARAMETER_STRUCT_REF(FFluidVolume, FluidVolume)
    SHADER_PARAMETER_STRUCT_REF(FFluidVolumeLocal, FluidBounds)

    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float3>, Positions)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<float3>, PredictedPositions)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint3>, SpatialIndices) //uint3(index, hash, key)
    SHADER_PARAMETER_RDG_BUFFER_SRV(StructuredBuffer<uint>, SpatialOffsets)
    SHADER_PARAMETER(uint32, NumParticles)
    SHADER_PARAMETER(float, SmoothingRadius)

    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture3D<float>, DensityMap)
    SHADER_PARAMETER(FUintVector3, DensityMapSize)

END_SHADER_PARAMETER_STRUCT()

// FluidMarch
BEGIN_SHADER_PARAMETER_STRUCT(FFluidMarchParams, )
    SHADER_PARAMETER_STRUCT_REF(FFluidVolume, FluidVolume)
    SHADER_PARAMETER_STRUCT_REF(FFluidVolumeLocal, FluidBounds)
    SHADER_PARAMETER_STRUCT_REF(FFluidEnvironment, Enviroment)
    SHADER_PARAMETER_STRUCT_REF(FViewUniformShaderParameters, View)
    
    SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float3>, Target)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneColor)
    SHADER_PARAMETER_RDG_TEXTURE(Texture2D, SceneDepth)
    
    SHADER_PARAMETER_RDG_TEXTURE_SRV(Texture3D<float>, DensityMap)
    SHADER_PARAMETER(FUintVector3, DensityMapSize)
END_SHADER_PARAMETER_STRUCT()



namespace RenderDispatch
{
    FRDGPassRef GenerateDensityMap(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FRenderPrepParams Params);
    FRDGPassRef Raymarch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMarchParams Params);
}

namespace FluidMathDispatch
{
    FRDGPassRef ExternalForces(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef UpdateSpatialLookup(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef SortAndCalculateOffsets(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef CalculateDensity(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef CalculatePressureForce(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef CalculateViscosityForce(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef UpdatePositions(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef ResolveBoxCollision(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef ResolveSphereCollision(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
    FRDGPassRef ResolveCapsuleCollision(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params);
}


namespace Shaders
{
    /// Shader Classes

    // ---------
    // F*ShaderName*Shader
    // ... using FParameters = F*ShaderName*ShaderParams
    // ---------

    class FFluidMathExternalForces : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathExternalForces);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathExternalForces, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathCalculateDensity : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathCalculateDensity);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathCalculateDensity, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathCalculatePressureForce : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathCalculatePressureForce);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathCalculatePressureForce, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathCalculateViscosityForce : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathCalculateViscosityForce);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathCalculateViscosityForce, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };
    
    class FFluidMathUpdateSpatialLookup : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathUpdateSpatialLookup);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathUpdateSpatialLookup, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathSortSpatialLookup : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathSortSpatialLookup);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathSortSpatialLookup, FGlobalShader);

    	using FParameters = FFluidGPUSortParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };
      
    class FFluidMathCalculateOffsets : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathCalculateOffsets);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathCalculateOffsets, FGlobalShader);

    	using FParameters = FFluidGPUSortParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathUpdatePositions : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathUpdatePositions);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathUpdatePositions, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathResolveBoxCollision : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathResolveBoxCollision);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathResolveBoxCollision, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathResolveSphereCollision : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathResolveSphereCollision);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathResolveSphereCollision, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FFluidMathResolveCapsuleCollision : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FFluidMathResolveCapsuleCollision);
    	SHADER_USE_PARAMETER_STRUCT(FFluidMathResolveCapsuleCollision, FGlobalShader);

    	using FParameters = FFluidMathParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 32);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 1);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

    class FRenderPrepShader : public FGlobalShader
    {
    public:
    	DECLARE_GLOBAL_SHADER(FRenderPrepShader);
    	SHADER_USE_PARAMETER_STRUCT(FRenderPrepShader, FGlobalShader);

    	using FParameters = FRenderPrepParams;

        // Basic shader initialization
        static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters) {
            return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
        }

    	static void ModifyCompilationEnvironment(const FGlobalShaderPermutationParameters& Parameters, FShaderCompilerEnvironment& OutEnvironment)
    	{
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 8);
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
    		OutEnvironment.SetDefine(TEXT("THREADS_X"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Y"), 8);
    		OutEnvironment.SetDefine(TEXT("THREADS_Z"), 1);
    	}
    };

} // namespace Shaders