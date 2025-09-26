#include "ParticleLibrary.h"
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
#include "CoreMinimal.h"
#include "Shader.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"


void UParticleBuffers::Initialize( 
    uint32 _NumParticles,
    FFluidVolumeLocal VolumeBounds
)
{
    NumParticles = _NumParticles;

    ENQUEUE_RENDER_COMMAND(ParticleBufferInit)(
    [this, _NumParticles, VolumeBounds](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        UE_LOG(LogTemp, Warning, TEXT("NUM PARTICLES: %d"), _NumParticles);
        // Create External Particle Buffers | make them persistent :3
        FRDGBufferDesc PositionsDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), _NumParticles);
        FRDGBufferDesc PredictedPositionsDesc   = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), _NumParticles);
        FRDGBufferDesc VelocitiesDesc           = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), _NumParticles);
        FRDGBufferDesc DensitiesDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(float), _NumParticles);
        FRDGBufferDesc SpatialIndicesDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(FUintVector3), _NumParticles);
        FRDGBufferDesc SpatialOffsetsDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), _NumParticles);

        FRDGBufferRef TmpPositionsRef            = GraphBuilder.CreateBuffer(PositionsDesc,             TEXT("Atlas Positions"));
        FRDGBufferRef TmpPredictedPositionsRef   = GraphBuilder.CreateBuffer(PredictedPositionsDesc,    TEXT("Atlas PredictedPositions"));
        FRDGBufferRef TmpVelocitiesRef           = GraphBuilder.CreateBuffer(VelocitiesDesc,            TEXT("Atlas Velocities"));
        FRDGBufferRef TmpDensitiesRef            = GraphBuilder.CreateBuffer(DensitiesDesc,             TEXT("Atlas Densities"));
        FRDGBufferRef TmpSpatialIndicesRef       = GraphBuilder.CreateBuffer(SpatialIndicesDesc,        TEXT("Atlas SpatialIndices"));
        FRDGBufferRef TmpSpatialOffsetsRef       = GraphBuilder.CreateBuffer(SpatialOffsetsDesc,        TEXT("Atlas SpatialOffsets"));

        Positions             = GraphBuilder.ConvertToExternalBuffer(TmpPositionsRef         );
        PredictedPositions    = GraphBuilder.ConvertToExternalBuffer(TmpPredictedPositionsRef);
        Velocities            = GraphBuilder.ConvertToExternalBuffer(TmpVelocitiesRef        );
        Densities             = GraphBuilder.ConvertToExternalBuffer(TmpDensitiesRef         );
        SpatialIndices        = GraphBuilder.ConvertToExternalBuffer(TmpSpatialIndicesRef    );
        SpatialOffsets        = GraphBuilder.ConvertToExternalBuffer(TmpSpatialOffsetsRef    );

        PositionsRef            = GraphBuilder.RegisterExternalBuffer(Positions         ,   TEXT("Atlas Positions"));
        PredictedPositionsRef   = GraphBuilder.RegisterExternalBuffer(PredictedPositions,   TEXT("Atlas PredictedPositions"));
        VelocitiesRef           = GraphBuilder.RegisterExternalBuffer(Velocities        ,   TEXT("Atlas Velocities"));
        DensitiesRef            = GraphBuilder.RegisterExternalBuffer(Densities         ,   TEXT("Atlas Densities"));
        SpatialIndicesRef       = GraphBuilder.RegisterExternalBuffer(SpatialIndices    ,   TEXT("Atlas SpatialIndices"));
        SpatialOffsetsRef       = GraphBuilder.RegisterExternalBuffer(SpatialOffsets    ,   TEXT("Atlas SpatialOffsets"));
        
        // Upload Default Values
        TArray<FVector3f> _positions;
        TArray<FVector3f> _preditctedpositions;
        TArray<FVector3f> _velocities;
        TArray<float> _densities;
        TArray<FUintVector3> _spatialindicies;
        TArray<uint32> _spatialoffsets;

        _positions.Init(FVector3f(1,1,1), _NumParticles);
        _preditctedpositions.Init(FVector3f(1,1,1), _NumParticles);
        _velocities.Init(FVector3f(1,1,1), _NumParticles);
        _densities.Init(float(1), _NumParticles);
        _spatialindicies.Init(FUintVector3(1,1,1), _NumParticles);
        _spatialoffsets.Init(uint32(1), _NumParticles);

        GraphBuilder.QueueBufferUpload(PositionsRef, _positions.GetData(), _positions.NumBytes());
        GraphBuilder.QueueBufferUpload(PredictedPositionsRef,_preditctedpositions.GetData(), _preditctedpositions.NumBytes());
        GraphBuilder.QueueBufferUpload(VelocitiesRef,_velocities.GetData(), _velocities.NumBytes());
        GraphBuilder.QueueBufferUpload(DensitiesRef,_densities.GetData(), _densities.NumBytes());
        GraphBuilder.QueueBufferUpload(SpatialIndicesRef,_spatialindicies.GetData(), _spatialindicies.NumBytes());
        GraphBuilder.QueueBufferUpload(SpatialOffsetsRef,_spatialoffsets.GetData(), _spatialoffsets.NumBytes());

        FluidBoundsLocal.MinBounds = VolumeBounds.MinBounds;
        FluidBoundsLocal.MaxBounds = VolumeBounds.MaxBounds;

        GraphBuilder.Execute();
        UE_LOG(LogTemp, Warning, TEXT("finisheesd particles init"));
    });

    bInitialized = true;
}

UParticleBuffers::~UParticleBuffers()
{
    // DELETE THE BUFFERS IF POSSIBLE PLEASE !!!!!!!!!!!111!!!11Elf!!!
}

void UParticleBuffers::Register(FRDGBuilder& GraphBuilder)
{
    PositionsRef            = GraphBuilder.RegisterExternalBuffer(Positions, TEXT("Atlas Positions"));
    PredictedPositionsRef   = GraphBuilder.RegisterExternalBuffer(PredictedPositions, TEXT("Atlas PredictedPositions"));
    VelocitiesRef           = GraphBuilder.RegisterExternalBuffer(Velocities, TEXT("Atlas Velocities"));
    DensitiesRef            = GraphBuilder.RegisterExternalBuffer(Densities, TEXT("Atlas Densities"));
    SpatialIndicesRef       = GraphBuilder.RegisterExternalBuffer(SpatialIndices, TEXT("Atlas SpatialIndices"));
    SpatialOffsetsRef       = GraphBuilder.RegisterExternalBuffer(SpatialOffsets, TEXT("Atlas SpatialOffsets"));  
}

 void UParticleBuffers::CreateUAVs(
    FRDGBuilder& GraphBuilder,
    FRDGBufferUAV*& OutPositions, 
    FRDGBufferUAV*& OutPredictedPositions, 
    FRDGBufferUAV*& OutVelocities, 
    FRDGBufferUAV*& OutDensities, 
    FRDGBufferUAV*& OutSpatialIndices, 
    FRDGBufferUAV*& OutSpatialOffsets
)
{
    OutPositions = GraphBuilder.CreateUAV(PositionsRef);
    OutPredictedPositions = GraphBuilder.CreateUAV(PredictedPositionsRef);
    OutVelocities = GraphBuilder.CreateUAV(VelocitiesRef);
    OutDensities = GraphBuilder.CreateUAV(DensitiesRef);
    OutSpatialIndices = GraphBuilder.CreateUAV(SpatialIndicesRef);
    OutSpatialOffsets = GraphBuilder.CreateUAV(SpatialOffsetsRef);
}

 void UParticleBuffers::CreateSRVs(
    FRDGBuilder& GraphBuilder,
    FRDGBufferSRV*& OutPositions
)
{
    OutPositions = GraphBuilder.CreateSRV(PositionsRef);
    //OutPredictedPositions = GraphBuilder.CreateSRV(PredictedPositionsRef);
    //OutVelocities = GraphBuilder.CreateSRV(VelocitiesRef);
    //OutDensities = GraphBuilder.CreateSRV(DensitiesRef);
    //OutSpatialIndices = GraphBuilder.CreateSRV(SpatialIndicesRef);
    //OutSpatialOffsets = GraphBuilder.CreateSRV(SpatialOffsetsRef);
}

FFluidMathParams UParticleBuffers::GetParticleParameters(FRDGBuilder& GraphBuilder)
{
    FFluidMathParams FluidMath;

    CreateUAVs(
        GraphBuilder,
        FluidMath.Positions, 
        FluidMath.PredictedPositions, 
        FluidMath.Velocities, 
        FluidMath.Densities, 
        FluidMath.SpatialIndices, 
        FluidMath.SpatialOffsets
    );
    FluidMath.Volume = TUniformBufferRef<FFluidVolumeLocal>::CreateUniformBufferImmediate(FluidBoundsLocal, EUniformBufferUsage::UniformBuffer_SingleFrame);  

    FluidMath.CollisionDampening = 0.6f;
    FluidMath.DeltaTime = 1.f/60.f;
    FluidMath.Gravity = -98.1f;
    FluidMath.NumParticles = 500;
    FluidMath.PressureAmplifier = 100.f;
    FluidMath.SmoothingRadius = 4.f;
    FluidMath.TargetDensity = 3.f;
    FluidMath.ViscosityStrength = 1.f;

    return FluidMath;
}

FRDGBufferSRVRef UParticleBuffers::GetRenderPrepParameters(FRDGBuilder& GraphBuilder)
{
    // Might want to expand this if we need more params in Renderprep
    FRDGBufferSRVRef SRV;
    CreateSRVs(GraphBuilder, SRV);
    return SRV;
}