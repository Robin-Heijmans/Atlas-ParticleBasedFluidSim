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
    
    IMPLEMENT_GLOBAL_SHADER(FParticleSimulationShader,  "/Shaders/Compute/ParticleSim.usf", "Compute", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FRenderPrepShader,          "/Shaders/Compute/RenderPrep.usf", "Compute", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMarchShader,          "/Shaders/Compute/FluidMarch.usf", "Compute", SF_Compute);

    // ... add new implemenations here
}

// Global Shader Buffers
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFluidVolume, "FluidVolume");
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticles, "FluidParticles");


// Dispatch Functions ...
void FParticleSimulationDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FParticles& Particles)
{
    RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation");

    Shaders::FParticleSimulationShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FParticleSimulationShader::FParameters>();
    PassParameters->Positions = Particles.Positions;
    PassParameters->NumParticles = Particles.NumParticles;
    
    const FIntVector DispatchCount(X,Y,Z);
    TShaderMapRef<Shaders::FParticleSimulationShader> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute ParticleSimulation"), 
        ComputeShader,
        PassParameters,
        DispatchCount);
}

void FRenderPrepDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, TUniformBufferRef<FParticles>& Particles)
{
    RDG_EVENT_SCOPE(GraphBuilder, "RenderPrep");

    Shaders::FRenderPrepShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FRenderPrepShader::FParameters>();
    //PassParameters->Particles = Particles;

    const FIntVector DispatchCount(X,Y,Z);
    TShaderMapRef<Shaders::FRenderPrepShader> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute RenderPrep"),
        ComputeShader,
        PassParameters,
        DispatchCount);
}


void FFluidMarchDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, const FSceneView& InView, FRDGTexture* SceneColor, FFluidVolume& Volume)  
{
    RDG_EVENT_SCOPE(GraphBuilder, "FluidMarch");
 
    Shaders::FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMarchShader::FParameters>();

    FRDGTextureDesc OutputDesc {};
    OutputDesc = SceneColor->Desc;
    OutputDesc.Reset();
    OutputDesc.Flags |= TexCreate_UAV;
    OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
    const FLinearColor ClearColor(0., 0., 0., 0.);
    OutputDesc.ClearValue = FClearValueBinding(ClearColor);

    const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("TanFluidShader_Output"));

    PassParameters->Target = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
    PassParameters->Volume = TUniformBufferRef<FFluidVolume>::CreateUniformBufferImmediate(Volume, EUniformBufferUsage::UniformBuffer_SingleFrame);
    PassParameters->SceneColor = SceneColor;
    PassParameters->View = InView.ViewUniformBuffer;

	const FIntPoint ViewSize = SceneColor->Desc.Extent;
    const FIntVector DispatchCount = FComputeShaderUtils::GetGroupCount(ViewSize, FComputeShaderUtils::kGolden2DGroupSize);
    
    TShaderMapRef<Shaders::FFluidMarchShader> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute TanComputeShader %dx%d", ViewSize.X, ViewSize.Y),
        ComputeShader,
        PassParameters,
        DispatchCount);

    AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor);

}