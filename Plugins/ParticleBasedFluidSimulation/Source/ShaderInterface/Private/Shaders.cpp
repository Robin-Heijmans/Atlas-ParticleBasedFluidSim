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
    
    //IMPLEMENT_GLOBAL_SHADER(FParticleSimulationShader,  "/Shaders/Compute/ParticleSim.usf", "Compute", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FRenderPrepShader,                  "/Shaders/Compute/RenderPrep.usf", "Compute", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMarchShader,                  "/Shaders/Compute/FluidMarch.usf", "Compute", SF_Compute);

    // Fluid Math Kernels
    IMPLEMENT_GLOBAL_SHADER(FFluidMathExternalForces,           "/Shaders/Compute/FluidMath.usf", "ExternalForces", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathUpdateSpatialLookup,      "/Shaders/Compute/FluidMath.usf", "UpdateSpatialLookup", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathSortSpatialLookup,        "/Shaders/Compute/BitonicMergeSort.usf", "Sort", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculateOffsets,         "/Shaders/Compute/BitonicMergeSort.usf", "CalculateOffsets", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculateDensity,         "/Shaders/Compute/FluidMath.usf", "CalculateDensity", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculatePressureForce,   "/Shaders/Compute/FluidMath.usf", "CalculatePressureForce", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculateViscosityForce,  "/Shaders/Compute/FluidMath.usf", "CalculateViscosityForce", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathUpdatePositions,          "/Shaders/Compute/FluidMath.usf", "UpdatePositions", SF_Compute);

    // ... add new implemenations here
}

// Global Shader Buffers
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFluidVolume, "FluidVolume");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFluidVolumeLocal, "Bounds");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFluidEnvironment, "Environment");

namespace FluidMathDispatch
{
    FRDGPassRef ExternalForces(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation ExternalForces");

        using ShaderType = Shaders::FFluidMathExternalForces;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute ExternalForces"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    FRDGPassRef UpdateSpatialLookup(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdateSpatialLookup");

        using ShaderType = Shaders::FFluidMathUpdateSpatialLookup;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathUpdateSpatialLookup::FParameters>();
        *PassParameters = Params;

        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);
        
        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdateSpatialLookup"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    FRDGPassRef SortAndCalculateOffsets(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdateSpatialLookup");

        using SorthaderType = Shaders::FFluidMathSortSpatialLookup;
        
        uint32 bufferCount = Params.NumParticles;
        

        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<SorthaderType> SortComputeShader(GlobalShaderMap);

        int numStages = static_cast<int>(FMath::Log2(static_cast<float>(FMath::RoundUpToPowerOfTwo(bufferCount))));
        FRDGPassRef oldPassRef = nullptr;
        FRDGPassRef currentPassRef;

        for (int stageIndex = 0; stageIndex < numStages; stageIndex++)
        {
            for (int stepIndex = 0; stepIndex < stageIndex + 1; stepIndex++)
            {
                SorthaderType::FParameters* PassParametersSort = GraphBuilder.AllocParameters<Shaders::FFluidMathSortSpatialLookup::FParameters>();
                PassParametersSort->Entries = Params.SpatialIndices;
                PassParametersSort->Offsets = Params.SpatialOffsets;
                PassParametersSort->numEntries = bufferCount;

                // Calculate some pattern stuff
                int groupWidth = 1 << (stageIndex - stepIndex);
                int groupHeight = 2 * groupWidth - 1;
                PassParametersSort->groupWidth = groupWidth;
                PassParametersSort->groupHeight = groupHeight;
                PassParametersSort->stepIndex = stepIndex;
                // Run the sorting step on the GPU
                currentPassRef = FComputeShaderUtils::AddPass(
                    GraphBuilder,
                    RDG_EVENT_NAME("Sort spatial lookup table"), 
                    SortComputeShader,
                    PassParametersSort,
                    FIntVector(FMath::RoundUpToPowerOfTwo(bufferCount) / 128, 1, 1));
                if (oldPassRef) {
                    GraphBuilder.AddPassDependency(oldPassRef, currentPassRef);
                    UE_LOG(LogTemp, Warning, TEXT("Jow"));
                }
                oldPassRef = currentPassRef;
                //ComputeHelper.Dispatch(sortCompute, FMath::RoundUpToPowerOfTwo(bufferCount) / 2);
            }
        }
        using OffsetShaderType = Shaders::FFluidMathCalculateOffsets;
        TShaderMapRef<OffsetShaderType> OffsetComputeShader(GlobalShaderMap);
        OffsetShaderType::FParameters* PassParametersOffset = GraphBuilder.AllocParameters<Shaders::FFluidMathSortSpatialLookup::FParameters>();
        PassParametersOffset->Entries = Params.SpatialIndices;
        PassParametersOffset->Offsets = Params.SpatialOffsets;
        PassParametersOffset->numEntries = bufferCount;
        PassParametersOffset->groupWidth = 0;
        PassParametersOffset->groupHeight = 0;
        PassParametersOffset->stepIndex = 0;

        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdateSpatialLookup"), 
            ERDGPassFlags::Compute,
            OffsetComputeShader,
            PassParametersOffset,
            DispatchCount);
    }
    FRDGPassRef CalculateDensity(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculateDensity");

        using ShaderType = Shaders::FFluidMathCalculateDensity;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculateDensity"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    FRDGPassRef CalculatePressureForce(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculatePressureForce");

        using ShaderType = Shaders::FFluidMathCalculatePressureForce;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculatePressureForce"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    FRDGPassRef CalculateViscosityForce(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculateViscosityForce");

        using ShaderType = Shaders::FFluidMathCalculateViscosityForce;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculateViscosityForce"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    FRDGPassRef UpdatePositions(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdatePositions");

        using ShaderType = Shaders::FFluidMathUpdatePositions;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;

        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(32)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        return FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdatePositions"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
}

void FRenderPrepDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FRenderPrepParams Params)
{
    RDG_EVENT_SCOPE(GraphBuilder, "Atlas RenderPrep");
   
    using ShaderType = Shaders::FRenderPrepShader;
    ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<ShaderType::FParameters>();
    *PassParameters = Params;

    const FIntVector DispatchCount(Params.DensityMapSize / 8);
    TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute Atlas RenderPrep"),
        ERDGPassFlags::AsyncCompute,
        ComputeShader,
        PassParameters,
        DispatchCount);
}

void FFluidMarchDispatchParams::Dispatch(
    FRDGBuilder& GraphBuilder, 
    FGlobalShaderMap* GlobalShaderMap, 
    FFluidMarchParams Params)  
{
    RDG_EVENT_SCOPE(GraphBuilder, "Atlas FluidMarch");
 
    using ShaderType = Shaders::FFluidMarchShader;
    ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<ShaderType::FParameters>();

    //DensityMap Input
    *PassParameters = Params;

	const FIntPoint ViewSize = PassParameters->SceneColor->Desc.Extent;
    const FIntVector DispatchCount = FComputeShaderUtils::GetGroupCount(ViewSize, FComputeShaderUtils::kGolden2DGroupSize);
    
    TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);
    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute Atlas FluidMarch %dx%d", ViewSize.X, ViewSize.Y),
        ERDGPassFlags::AsyncCompute,
        ComputeShader,
        PassParameters,
        DispatchCount);
}
