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

#include "FluidBoundingVolume.h"

void UParticleBuffers::Initialize( 
    TUniformBufferRef<FFluidVolumeLocal> VolumeBounds,
    const AFluidBoundingVolume* Volume
)
{
    ENQUEUE_RENDER_COMMAND(ParticleBufferInit)(
    [this, VolumeBounds, Volume](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        // Create External Particle Buffers | make them persistent :3
        
        SimulationSettings.NumParticles = Volume->Simulation->GetParticles().Num();
        FRDGBufferDesc PositionsDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f),       SimulationSettings.NumParticles);
        FRDGBufferDesc PredictedPositionsDesc   = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f),       SimulationSettings.NumParticles);
        FRDGBufferDesc VelocitiesDesc           = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f),       SimulationSettings.NumParticles);
        FRDGBufferDesc DensitiesDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(float),           SimulationSettings.NumParticles);
        FRDGBufferDesc SpatialIndicesDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(FUintVector3),    SimulationSettings.NumParticles);
        FRDGBufferDesc SpatialOffsetsDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32),          SimulationSettings.NumParticles);

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
        
        // Particle Buffer Parameters
        TArray<FVector3f> _positions;
        TArray<FVector3f> _preditctedpositions;
        TArray<FVector3f> _velocities;
        TArray<float> _densities;
        TArray<FUintVector3> _spatialindicies;
        TArray<uint32> _spatialoffsets;

        // Upload Initial Vlaues
        uint32 i = 0;
        for (const FParticle& Particle : Volume->Simulation->GetParticles())
        {
            //UE_LOG(LogTemp, Warning, TEXT("Particle Position %d: %f,%f,%f"), i, Particle.Position.X, Particle.Position.Y, Particle.Position.Z);
            
            _positions.Add(FVector3f(Particle.Position));
            _preditctedpositions.Add(FVector3f(Particle.PredictedPosition));
            _velocities.Add(FVector3f(Particle.Velocity));
            _densities.Add(Particle.Density);
            _spatialindicies.Add(FUintVector3(-1));
            _spatialoffsets.Add(uint32(-1));
        }
    
        GraphBuilder.QueueBufferUpload(PositionsRef, _positions.GetData(), _positions.NumBytes());
        GraphBuilder.QueueBufferUpload(PredictedPositionsRef,_positions.GetData(), _positions.NumBytes());
        GraphBuilder.QueueBufferUpload(VelocitiesRef,_velocities.GetData(), _velocities.NumBytes());
        GraphBuilder.QueueBufferUpload(DensitiesRef,_densities.GetData(), _densities.NumBytes());
        GraphBuilder.QueueBufferUpload(SpatialIndicesRef,_spatialindicies.GetData(), _spatialindicies.NumBytes());
        GraphBuilder.QueueBufferUpload(SpatialOffsetsRef,_spatialoffsets.GetData(), _spatialoffsets.NumBytes());

        // Generate Spatial indices and offsets
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        FFluidMathParams FluidMath = GetParticleParameters(GraphBuilder);
        FluidMath.FluidBounds = VolumeBounds;
        
        FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, FluidMath);
        FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, FluidMath);
        FluidMathDispatch::SortAndCalculateOffsets(GraphBuilder, GlobalShaderMap, FluidMath);
        FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, FluidMath);
        FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, FluidMath);
        FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, FluidMath);
        FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, FluidMath);

        GraphBuilder.Execute();
    });

    bInitialized = true;
}

UParticleBuffers::~UParticleBuffers()
{
    // DELETE THE BUFFERS IF POSSIBLE PLEASE !!!!!!!!!!!111!!!11Elf!!!
    if(Positions && Positions.IsValid())                    Positions->Release();
	if(PredictedPositions && PredictedPositions.IsValid())  PredictedPositions->Release();
	if(Velocities && Velocities.IsValid())                  Velocities->Release();
	if(Densities && Densities.IsValid())                    Densities->Release();
	if(SpatialIndices && SpatialIndices.IsValid())          SpatialIndices->Release();
	if(SpatialOffsets && SpatialOffsets.IsValid())          SpatialOffsets->Release();
    bInitialized = false;
}

void UParticleBuffers::Register(FRDGBuilder& GraphBuilder)
{
    if(bInitialized)
    {
        PositionsRef            = GraphBuilder.RegisterExternalBuffer(Positions, TEXT("Atlas Positions"));
        PredictedPositionsRef   = GraphBuilder.RegisterExternalBuffer(PredictedPositions, TEXT("Atlas PredictedPositions"));
        VelocitiesRef           = GraphBuilder.RegisterExternalBuffer(Velocities, TEXT("Atlas Velocities"));
        DensitiesRef            = GraphBuilder.RegisterExternalBuffer(Densities, TEXT("Atlas Densities"));
        SpatialIndicesRef       = GraphBuilder.RegisterExternalBuffer(SpatialIndices, TEXT("Atlas SpatialIndices"));
        SpatialOffsetsRef       = GraphBuilder.RegisterExternalBuffer(SpatialOffsets, TEXT("Atlas SpatialOffsets"));  
    }
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
    FRDGBufferSRV*& OutPositions,
    FRDGBufferSRV*& OutPredictedPositions,
    FRDGBufferSRV*& OutSpatialIndices, 
    FRDGBufferSRV*& OutSpatialOffsets
)
{
    OutPositions = GraphBuilder.CreateSRV(PositionsRef);
    OutPredictedPositions = GraphBuilder.CreateSRV(PredictedPositionsRef);
    //OutVelocities = GraphBuilder.CreateSRV(VelocitiesRef);
    //OutDensities = GraphBuilder.CreateSRV(DensitiesRef);
    OutSpatialIndices = GraphBuilder.CreateSRV(SpatialIndicesRef);
    OutSpatialOffsets = GraphBuilder.CreateSRV(SpatialOffsetsRef);
}

FFluidMathParams UParticleBuffers::GetParticleParameters(FRDGBuilder& GraphBuilder)
{
    FFluidMathParams FluidMath;
    if(!bInitialized) return FluidMath;

    CreateUAVs(
        GraphBuilder,
        FluidMath.Positions, 
        FluidMath.PredictedPositions, 
        FluidMath.Velocities, 
        FluidMath.Densities, 
        FluidMath.SpatialIndices, 
        FluidMath.SpatialOffsets
    );

    FluidMath.CollisionDampening    = SimulationSettings.CollisionDampening    ;
    FluidMath.DeltaTime             = SimulationSettings.DeltaTime             ;
    FluidMath.Gravity               = SimulationSettings.Gravity               ;
    FluidMath.NumParticles          = SimulationSettings.NumParticles          ;
    FluidMath.PressureAmplifier     = SimulationSettings.PressureAmplifier     ;
    FluidMath.SmoothingRadius       = SimulationSettings.SmoothingRadius       ;
    FluidMath.TargetDensity         = SimulationSettings.TargetDensity         ;
    FluidMath.ViscosityStrength     = SimulationSettings.ViscosityStrength     ;

    return FluidMath;
}

FRenderPrepParams UParticleBuffers::GetRenderPrepParameters(FRDGBuilder& GraphBuilder)
{
    FRenderPrepParams RenderPrep;
    if(!bInitialized) return RenderPrep;
    
    // Might want to expand this if we need more params in Renderprep
    CreateSRVs(GraphBuilder, RenderPrep.Positions, RenderPrep.PredictedPositions, RenderPrep.SpatialIndices, RenderPrep.SpatialOffsets);
    RenderPrep.NumParticles = SimulationSettings.NumParticles;

    return RenderPrep;
}