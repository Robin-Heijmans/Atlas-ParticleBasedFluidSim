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
//IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticles, "FluidParticles");

namespace FluidMathDispatch
{
    void ExternalForces(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation ExternalForces");

        using ShaderType = Shaders::FFluidMathExternalForces;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute ExternalForces"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void UpdateSpatialLookup(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdateSpatialLookup");

        using ShaderType = Shaders::FFluidMathUpdateSpatialLookup;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathUpdateSpatialLookup::FParameters>();
        *PassParameters = Params;

        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);
        
        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdateSpatialLookup"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void SortAndCalculateOffsets(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdateSpatialLookup");

        using SorthaderType = Shaders::FFluidMathSortSpatialLookup;
        SorthaderType::FParameters* PassParametersSort = GraphBuilder.AllocParameters<Shaders::FFluidMathSortSpatialLookup::FParameters>();
        
        PassParametersSort->Entries = Params.SpatialIndices;
        PassParametersSort->Offsets = Params.SpatialOffsets;
        uint32 bufferCount = Params.NumParticles;
        PassParametersSort->numEntries = bufferCount;

        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<SorthaderType> SortComputeShader(GlobalShaderMap);

        int numStages = static_cast<int>(FMath::Log2(static_cast<float>(FMath::RoundUpToPowerOfTwo(bufferCount))));

        for (int stageIndex = 0; stageIndex < numStages; stageIndex++)
        {
            for (int stepIndex = 0; stepIndex < stageIndex + 1; stepIndex++)
            {
                // Calculate some pattern stuff
                int groupWidth = 1 << (stageIndex - stepIndex);
                int groupHeight = 2 * groupWidth - 1;
                PassParametersSort->groupWidth = groupWidth;
                PassParametersSort->groupHeight = groupHeight;
                PassParametersSort->stepIndex = stepIndex;
                // Run the sorting step on the GPU
                FComputeShaderUtils::AddPass(
                    GraphBuilder,
                    RDG_EVENT_NAME("Sort spatial lookup table"), 
                    SortComputeShader,
                    PassParametersSort,
                    DispatchCount);
                //ComputeHelper.Dispatch(sortCompute, FMath::RoundUpToPowerOfTwo(indexBuffer.count) / 2);
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

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdateSpatialLookup"), 
            ERDGPassFlags::Compute,
            OffsetComputeShader,
            PassParametersOffset,
            DispatchCount);
    }
    void CalculateDensity(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculateDensity");

        using ShaderType = Shaders::FFluidMathCalculateDensity;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculateDensity"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void CalculatePressureForce(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculatePressureForce");

        using ShaderType = Shaders::FFluidMathCalculatePressureForce;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculatePressureForce"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void CalculateViscosityForce(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculateViscosityForce");

        using ShaderType = Shaders::FFluidMathCalculateViscosityForce;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculateViscosityForce"), 
            ERDGPassFlags::Compute,
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void UpdatePositions(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdatePositions");

        using ShaderType = Shaders::FFluidMathUpdatePositions;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;

        const FIntVector DispatchCount(FMath::DivideAndRoundUp(Params.NumParticles, uint32(64)), 1, 1);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
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

    Shaders::FRenderPrepShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FRenderPrepShader::FParameters>();
    *PassParameters = Params;

    const FIntVector DispatchCount(32,32,32);
    TShaderMapRef<Shaders::FRenderPrepShader> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute RenderPrep"),
        ERDGPassFlags::Compute,
        ComputeShader,
        PassParameters,
        DispatchCount);
}

void FFluidMarchDispatchParams::Dispatch(
    FRDGBuilder& GraphBuilder, 
    FGlobalShaderMap* GlobalShaderMap, 
    const FSceneView& InView, 
    FRDGTexture* SceneColor, 
    FFluidVolume& Volume, 
    const FRDGTextureRef& DensityMapRef)  
{
    RDG_EVENT_SCOPE(GraphBuilder, "FluidMarch");
 
    Shaders::FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMarchShader::FParameters>();

    // Output Texture
    FRDGTextureDesc OutputDesc {};
    OutputDesc = SceneColor->Desc;
    //OutputDesc.Extent /= 4.0;
    OutputDesc.Reset();
    OutputDesc.Flags |= TexCreate_UAV;
    OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
    const FLinearColor ClearColor(0., 0., 0., 0.);
    OutputDesc.ClearValue = FClearValueBinding(ClearColor);

    const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("TanFluidShader_Output"));

    //DensityMap Input
    PassParameters->Target = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
    PassParameters->DensityMap = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(DensityMapRef));
    PassParameters->DensityMapSize = DensityMapRef->GetRHI()->GetSizeXYZ().Size();
    PassParameters->Volume = TUniformBufferRef<FFluidVolume>::CreateUniformBufferImmediate(Volume, EUniformBufferUsage::UniformBuffer_SingleFrame);
    PassParameters->SceneColor = SceneColor;
    PassParameters->View = InView.ViewUniformBuffer;

	const FIntPoint ViewSize = SceneColor->Desc.Extent;
    const FIntVector DispatchCount = FComputeShaderUtils::GetGroupCount(ViewSize, FIntPoint(48,16));
    
    TShaderMapRef<Shaders::FFluidMarchShader> ComputeShader(GlobalShaderMap);
    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute TanComputeShader %dx%d", ViewSize.X, ViewSize.Y),
        ERDGPassFlags::Compute,
        ComputeShader,
        PassParameters,
        DispatchCount);

    AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor);

}
