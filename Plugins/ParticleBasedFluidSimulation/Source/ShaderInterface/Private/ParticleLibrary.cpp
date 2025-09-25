#include "ParticleLibrary.h"





UParticleBuffers::UParticleBuffers( 
    const uint32 NumParticles,
    const FFluidVolumeLocal& VolumeBounds
)
{
    ENQUEUE_RENDER_COMMAND(GenDensityMap)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);
        {
            // Create External Particle Buffers | make them persistent :3
            FRDGBufferDesc PositionsDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), 500);
            FRDGBufferDesc PredictedPositionsDesc   = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), 500);
            FRDGBufferDesc VelocitiesDesc           = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), 500);
            FRDGBufferDesc DensitiesDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(float), 500);
            FRDGBufferDesc SpatialIndicesDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(FUintVector3), 500);
            FRDGBufferDesc SpatialOffsetsDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 500);

            FRDGBufferRef PositionsRef            = GraphBuilder.CreateBuffer(PositionsDesc,             TEXT("TanFluid Positions"));
            FRDGBufferRef PredictedPositionsRef   = GraphBuilder.CreateBuffer(PredictedPositionsDesc,    TEXT("TanFluid PredictedPositions"));
            FRDGBufferRef VelocitiesRef           = GraphBuilder.CreateBuffer(VelocitiesDesc,            TEXT("TanFluid Velocities"));
            FRDGBufferRef DensitiesRef            = GraphBuilder.CreateBuffer(DensitiesDesc,             TEXT("TanFluid Densities"));
            FRDGBufferRef SpatialIndicesRef       = GraphBuilder.CreateBuffer(SpatialIndicesDesc,        TEXT("TanFluid SpatialIndices"));
            FRDGBufferRef SpatialOffsetsRef       = GraphBuilder.CreateBuffer(SpatialOffsetsDesc,        TEXT("TanFluid SpatialOffsets"));

            Positions             = GraphBuilder.ConvertToExternalBuffer(PositionsRef         );
            PredictedPositions    = GraphBuilder.ConvertToExternalBuffer(PredictedPositionsRef);
            Velocities            = GraphBuilder.ConvertToExternalBuffer(VelocitiesRef        );
            Densities             = GraphBuilder.ConvertToExternalBuffer(DensitiesRef         );
            SpatialIndices        = GraphBuilder.ConvertToExternalBuffer(SpatialIndicesRef    );
            SpatialOffsets        = GraphBuilder.ConvertToExternalBuffer(SpatialOffsetsRef    );
        }

        const FRDGBufferRef PositionsRef            = GraphBuilder.RegisterExternalBuffer(Positions         ,   TEXT("TanFluid Positions"));
        const FRDGBufferRef PredictedPositionsRef   = GraphBuilder.RegisterExternalBuffer(PredictedPositions,   TEXT("TanFluid PredictedPositions"));
        const FRDGBufferRef VelocitiesRef           = GraphBuilder.RegisterExternalBuffer(Velocities        ,   TEXT("TanFluid Velocities"));
        const FRDGBufferRef DensitiesRef            = GraphBuilder.RegisterExternalBuffer(Densities         ,   TEXT("TanFluid Densities"));
        const FRDGBufferRef SpatialIndicesRef       = GraphBuilder.RegisterExternalBuffer(SpatialIndices    ,   TEXT("TanFluid SpatialIndices"));
        const FRDGBufferRef SpatialOffsetsRef       = GraphBuilder.RegisterExternalBuffer(SpatialOffsets    ,   TEXT("TanFluid SpatialOffsets"));
        
        // Upload Default Values
        TArray<FVector3f> _positions;
        TArray<FVector3f> _preditctedpositions;
        TArray<FVector3f> _velocities;
        TArray<float> _densities;
        TArray<FUintVector3> _spatialindicies;
        TArray<uint32> _spatialoffsets;

        _positions.Init(FVector3f(1,1,1), 500);
        _preditctedpositions.Init(FVector3f(1,1,1), 500);
        _velocities.Init(FVector3f(1,1,1), 500);
        _densities.Init(float(1), 500);
        _spatialindicies.Init(FUintVector3(1,1,1), 500);
        _spatialoffsets.Init(uint32(1), 500);

        GraphBuilder.QueueBufferUpload(PositionsRef, _positions.GetData(), _positions.NumBytes());
        GraphBuilder.QueueBufferUpload(PredictedPositionsRef,_preditctedpositions.GetData(), _preditctedpositions.NumBytes());
        GraphBuilder.QueueBufferUpload(VelocitiesRef,_velocities.GetData(), _velocities.NumBytes());
        GraphBuilder.QueueBufferUpload(DensitiesRef,_densities.GetData(), _densities.NumBytes());
        GraphBuilder.QueueBufferUpload(SpatialIndicesRef,_spatialindicies.GetData(), _spatialindicies.NumBytes());
        GraphBuilder.QueueBufferUpload(SpatialOffsetsRef,_spatialoffsets.GetData(), _spatialoffsets.NumBytes());

        FluidBoundsLocal.MinBounds = FVector3f(-32,-32,-32);
        FluidBoundsLocal.MaxBounds = FVector3f(32,32,32);
    });
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
    FRDGBufferUAV* OutPositions, 
    FRDGBufferUAV* OutPredictedPositions, 
    FRDGBufferUAV* OutVelocities, 
    FRDGBufferUAV* OutDensities, 
    FRDGBufferUAV* OutSpatialIndices, 
    FRDGBufferUAV* OutSpatialOffsets
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
    FRDGBufferSRV* OutPositions, 
    FRDGBufferSRV* OutPredictedPositions, 
    FRDGBufferSRV* OutVelocities, 
    FRDGBufferSRV* OutDensities, 
    FRDGBufferSRV* OutSpatialIndices, 
    FRDGBufferSRV* OutSpatialOffsets
)
{
    OutPositions = GraphBuilder.CreateSRV(PositionsRef);
    OutPredictedPositions = GraphBuilder.CreateSRV(PredictedPositionsRef);
    OutVelocities = GraphBuilder.CreateSRV(VelocitiesRef);
    OutDensities = GraphBuilder.CreateSRV(DensitiesRef);
    OutSpatialIndices = GraphBuilder.CreateSRV(SpatialIndicesRef);
    OutSpatialOffsets = GraphBuilder.CreateSRV(SpatialOffsetsRef);
}

FFluidMathParams UParticleBuffers::GetParticleParameters(FRDGBuilder& GraphBuilder)
{
    FFluidMathParams FluidMath;

    Register(GraphBuilder);
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
