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
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculateDensity,         "/Shaders/Compute/FluidMath.usf", "CalculateDensity", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculatePressureForce,   "/Shaders/Compute/FluidMath.usf", "CalculatePressureForce", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathCalculateViscosityForce,  "/Shaders/Compute/FluidMath.usf", "CalculateViscosityForce", SF_Compute);
    IMPLEMENT_GLOBAL_SHADER(FFluidMathUpdatePositions,          "/Shaders/Compute/FluidMath.usf", "UpdatePositions", SF_Compute);

    // ... add new implemenations here
}

// Global Shader Buffers
IMPLEMENT_UNIFORM_BUFFER_STRUCT(FFluidVolume, "FluidVolume");
//IMPLEMENT_UNIFORM_BUFFER_STRUCT(FParticles, "FluidParticles");

/*
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
*/

// Dispatch Functions ...
/*
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
        DispatchCount);//,ERDGPassFlags::Compute | ERDGPassFlags::NeverCull);
}       
*/

namespace FluidMathDispatch
{
    void ExternalForces(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params) 
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation ExternalForces");

        using ShaderType = Shaders::FFluidMathExternalForces;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(10,10,5);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute ExternalForces"), 
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void UpdateSpatialLookup(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation UpdateSpatialLookup");

        using ShaderType = Shaders::FFluidMathUpdateSpatialLookup;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(10,10,5);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdateSpatialLookup"), 
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
    void CalculateDensity(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams Params)
    {
        RDG_EVENT_SCOPE(GraphBuilder, "ParticleSimulation CalculateDensity");

        using ShaderType = Shaders::FFluidMathCalculateDensity;
        ShaderType::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FFluidMathExternalForces::FParameters>();
        *PassParameters = Params;
        
        const FIntVector DispatchCount(10,10,5);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculateDensity"), 
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
        
        const FIntVector DispatchCount(10,10,5);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculatePressureForce"), 
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
        
        const FIntVector DispatchCount(10,10,5);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute CalculateViscosityForce"), 
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
        
        const FIntVector DispatchCount(10,10,5);
        TShaderMapRef<ShaderType> ComputeShader(GlobalShaderMap);

        FComputeShaderUtils::AddPass(
            GraphBuilder,
            RDG_EVENT_NAME("Execute UpdatePositions"), 
            ComputeShader,
            PassParameters,
            DispatchCount);
    }
}

void FRenderPrepDispatchParams::Dispatch(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, const FRDGTextureRef& DensityMapRef)
{
    RDG_EVENT_SCOPE(GraphBuilder, "RenderPrep");

    Shaders::FRenderPrepShader::FParameters* PassParameters = GraphBuilder.AllocParameters<Shaders::FRenderPrepShader::FParameters>();
    PassParameters->DensityMap = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DensityMapRef));
    PassParameters->DensityMapSize = DensityMapRef->GetRHI()->GetSizeXYZ().Size();

    const FIntVector DispatchCount(64,64,64);
    TShaderMapRef<Shaders::FRenderPrepShader> ComputeShader(GlobalShaderMap);

    FComputeShaderUtils::AddPass(
        GraphBuilder,
        RDG_EVENT_NAME("Execute RenderPrep"),
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
