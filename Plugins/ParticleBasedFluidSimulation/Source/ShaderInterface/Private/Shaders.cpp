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
    
    IMPLEMENT_GLOBAL_SHADER(FParticleSimulationShader,  "/Shaders/Compute/SimpleTest.usf", "ExternalForces", SF_Compute);
    //IMPLEMENT_GLOBAL_SHADER(FRenderPrepShader,          "/Shaders/Compute/RenderPrep.usf", "Compute", SF_Compute);
    //IMPLEMENT_GLOBAL_SHADER(FFluidMarchShader,          "/Shaders/Compute/FluidMarch.usf", "Compute", SF_Compute);

    // ... add new implemenations here
}

// Global Shader Buffers
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFluidVolume, "FluidVolume");
//IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticles, "FluidParticles");

void FParticleSimulationDispatchParams::CreateBuffers(FRDGBuilder& GraphBuilder, const TArray<FVector3f>& Positions)
{
    int NumParticles = Positions.Num();
    
    auto CreateStructuredBuffer = [&](int ElementSize, const TCHAR* Name, const void* InitialData = nullptr)
    {
        FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(ElementSize, NumParticles);
        FRDGBufferRef Buffer = GraphBuilder.CreateBuffer(Desc, Name);

        if (InitialData)
        {
            GraphBuilder.QueueBufferUpload(Buffer, InitialData, NumParticles * ElementSize);
        }

        FRDGBufferUAVRef UAV = GraphBuilder.CreateUAV(Buffer);
        return TTuple<FRDGBufferRef, FRDGBufferUAVRef>(Buffer, UAV);
    };

    

    if (!PositionRHI|| !VelocityRHI) {
        FRHIResourceCreateInfo Info(TEXT("ParticlePositions"));
        PositionRHI = RHICreateStructuredBuffer(sizeof(FVector3f), sizeof(FVector3f) * NumParticles, static_cast<uint32>(BUF_UnorderedAccess | BUF_ShaderResource), Info);
        FMemory::Memcpy(PositionRHI, Positions.GetData(), sizeof(FVector3f) * NumParticles);

        FRHIResourceCreateInfo Info(TEXT("ParticleVelocities"));
        VelocityRHI = RHICreateStructuredBuffer(sizeof(FVector3f), sizeof(FVector3f) * NumParticles, static_cast<uint32>(BUF_UnorderedAccess | BUF_ShaderResource), Info);
    }

    FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), NumParticles);
    TRefCountPtr<FRDGPooledBuffer> PooledBuffer = new FRDGPooledBuffer(PositionRHI, Desc, NumParticles, TEXT("ParticlePositions"));
    FRDGBufferRef PositionBuffer = GraphBuilder.RegisterExternalBuffer(PooledBuffer, TEXT("ParticlePositions"));
    PositionsUAV = GraphBuilder.CreateUAV(PositionBuffer);

    TRefCountPtr<FRDGPooledBuffer> PooledBuffer = new FRDGPooledBuffer(VelocityRHI, Desc, NumParticles, TEXT("ParticlePositions"));
    FRDGBufferRef VelocityBuffer = GraphBuilder.RegisterExternalBuffer(PooledBuffer, TEXT("ParticlePositions"));
    VelocitiesUAV = GraphBuilder.CreateUAV(VelocityBuffer);

    TTuple<FRDGBufferRef, FRDGBufferUAVRef> Buff;

    Buff = CreateStructuredBuffer(sizeof(FVector3f), TEXT("PredictedPositions"));
    PredictedPositionBuffer = Buff.Get<0>();
    PredictedPositionUAV = Buff.Get<1>();

    Buff = CreateStructuredBuffer(sizeof(float), TEXT("Densities"));
    DensityBuffer = Buff.Get<0>();
    DensityUAV = Buff.Get<1>();

    Buff = CreateStructuredBuffer(sizeof(FVector), TEXT("SpatialIndices"));
    SpatialIndicesBuffer = Buff.Get<0>();
    SpatialIndicesUAV = Buff.Get<1>();

    Buff = CreateStructuredBuffer(sizeof(int), TEXT("SpatialOffsets"));
    SpatialOffsetsBuffer = Buff.Get<0>();
    SpatialOffsetsUAV = Buff.Get<1>();

    BindBuffers(GraphBuilder, NumParticles);
}

void FParticleSimulationDispatchParams::BindBuffers(FRDGBuilder& GraphBuilder, const int& NumParticles) 
{
    PassParameters = GraphBuilder.AllocParameters<Shaders::FParticleSimulationShader::FParameters>();
    PassParameters->Positions = PositionsUAV;
    PassParameters->PredictedPositions = PredictedPositionUAV;
    PassParameters->Velocities = VelocitiesUAV;
    PassParameters->Densities = DensityUAV;
    PassParameters->SpatialIndices = SpatialIndicesUAV;
    PassParameters->SpatialOffsets = SpatialOffsetsUAV;

    PassParameters->CollisionDampening = 0.6f;
    PassParameters->DeltaTime = 1.f/60.f;
    PassParameters->Gravity = -98.1f;
    PassParameters->NumParticles = NumParticles;
    PassParameters->PressureAmplifier = 100.f;
    PassParameters->SmoothingRadius = 4.f;
    PassParameters->TargetDensity = 3.f;
    PassParameters->ViscosityStrength = 1.f;
}

// Dispatch Functions ...
void FParticleSimulationDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap)
{
    if (!PassParameters) return;
    RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation");

    const FIntVector DispatchCount(16,1,1);
    TShaderMapRef<Shaders::FParticleSimulationShader> ComputeShader(GlobalShaderMap);
    check(ComputeShader.IsValid());

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute ParticleSimulation"), 
        ComputeShader,
        PassParameters,
        DispatchCount);//,ERDGPassFlags::Compute | ERDGPassFlags::NeverCull);
}

//void FRenderPrepDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FParticles Particles)
//{
//    RDG_EVENT_SCOPE(GraphBuilder, "RenderPrep");
//
//    Shaders::FRenderPrepShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FRenderPrepShader::FParameters>();
//    PassParameters->Particles = TUniformBufferRef<FParticles>::CreateUniformBufferImmediate(Particles, EUniformBufferUsage::UniformBuffer_SingleFrame);
//
//    const FIntVector DispatchCount(1,1,1);
//    TShaderMapRef<Shaders::FRenderPrepShader> ComputeShader(GlobalShaderMap);
//
//    FComputeShaderUtils::AddPass(
//        GraphBuilder,
//        RDG_EVENT_NAME("Execute RenderPrep"),
//        ComputeShader,
//        PassParameters,
//        DispatchCount);
//}
//
//
//void FFluidMarchDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, const FSceneView& InView, FRDGTexture* SceneColor, FFluidVolume& Volume)  
//{
//    RDG_EVENT_SCOPE(GraphBuilder, "FluidMarch");
// 
//    Shaders::FFluidMarchShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMarchShader::FParameters>();
//
//    FRDGTextureDesc OutputDesc {};
//    OutputDesc = SceneColor->Desc;
//    OutputDesc.Reset();
//    OutputDesc.Flags |= TexCreate_UAV;
//    OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
//    const FLinearColor ClearColor(0., 0., 0., 0.);
//    OutputDesc.ClearValue = FClearValueBinding(ClearColor);
//
//    const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("TanFluidShader_Output"));
//
//    PassParameters->Target = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
//    PassParameters->Volume = TUniformBufferRef<FFluidVolume>::CreateUniformBufferImmediate(Volume, EUniformBufferUsage::UniformBuffer_SingleFrame);
//    PassParameters->SceneColor = SceneColor;
//    PassParameters->View = InView.ViewUniformBuffer;
//
//	const FIntPoint ViewSize = SceneColor->Desc.Extent;
//    const FIntVector DispatchCount = FComputeShaderUtils::GetGroupCount(ViewSize, FComputeShaderUtils::kGolden2DGroupSize);
//    
//    TShaderMapRef<Shaders::FFluidMarchShader> ComputeShader(GlobalShaderMap);
//
//    FComputeShaderUtils::AddPass(
//        GraphBuilder,
//        RDG_EVENT_NAME("Execute TanComputeShader %dx%d", ViewSize.X, ViewSize.Y),
//        ComputeShader,
//        PassParameters,
//        DispatchCount);
//
//    AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor);
//
//}