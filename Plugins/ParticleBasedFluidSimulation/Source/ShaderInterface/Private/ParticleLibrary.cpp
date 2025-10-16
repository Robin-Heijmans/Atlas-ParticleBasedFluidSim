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
#include "Components/BoxComponent.h"

#include "FluidBoundingVolume.h"

void UParticleBuffers::Initialize(AFluidBoundingVolume* Volume)
{
    ParentVolume = Volume;

    ENQUEUE_RENDER_COMMAND(ParticleBufferInit)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        // Creating Density Map
        FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("Atlas DensityMap"))
                .SetExtent(DensityMapSize.X, DensityMapSize.Y)
                .SetDepth(DensityMapSize.Z)
                .SetFormat(EPixelFormat::PF_R8)
                .SetFlags(ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource)
                .SetInitialState(ERHIAccess::SRVCompute);

        FTextureRHIRef DensityMapRHI = RHICreateTexture(Desc);
        DensityMap = CreateRenderTarget(DensityMapRHI, TEXT("Atlas DensityMap"));

        // Create External Particle Buffers | make them persistent :3
        
        SimulationSettings.NumParticles = ParentVolume->Simulation->GetParticles().Num();
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
        for (const FParticle& Particle : ParentVolume->Simulation->GetParticles())
        {
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
        DispatchFluidMath(GraphBuilder, GlobalShaderMap);

        GraphBuilder.Execute();
    });

    bInitialized = true;
}

UParticleBuffers::~UParticleBuffers()
{

}

void UParticleBuffers::OnUnregister()
{
    if(Positions && Positions.IsValid())                    Positions->Release();
	if(PredictedPositions && PredictedPositions.IsValid())  PredictedPositions->Release();
	if(Velocities && Velocities.IsValid())                  Velocities->Release();
	if(Densities && Densities.IsValid())                    Densities->Release();
	if(SpatialIndices && SpatialIndices.IsValid())          SpatialIndices->Release();
	if(SpatialOffsets && SpatialOffsets.IsValid())          SpatialOffsets->Release();
    bInitialized = false;

    Super::OnUnregister();
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

void UParticleBuffers::DispatchFluidMath(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap)
{
    //Update UBs
    FFluidVolumeLocal FluidVolumeLocal;
    FFluidVolume FluidVolume;
    
    TArray<FVector> VolumeBounds = ParentVolume->GetVolumeBounds();
    FluidVolumeLocal.MinBounds = FVector3f(VolumeBounds[0]);
    FluidVolumeLocal.MaxBounds = FVector3f(VolumeBounds[1]);
    
    FluidVolume.BoundsPosition = FVector3f(ParentVolume->Bounds->GetComponentLocation());
    FluidVolume.BoundsSize = FVector3f(ParentVolume->Bounds->GetScaledBoxExtent());
    
    UBFluidBounds = TUniformBufferRef<FFluidVolumeLocal>::CreateUniformBufferImmediate(FluidVolumeLocal, EUniformBufferUsage::UniformBuffer_SingleFrame);  
    UBFluidVolume = TUniformBufferRef<FFluidVolume>::CreateUniformBufferImmediate(FluidVolume, EUniformBufferUsage::UniformBuffer_SingleFrame);  
        
    // Fluid Math
    FFluidMathParams FluidMath;
    FluidMath.Positions = GraphBuilder.CreateUAV(PositionsRef);
    FluidMath.PredictedPositions = GraphBuilder.CreateUAV(PredictedPositionsRef);
    FluidMath.Velocities = GraphBuilder.CreateUAV(VelocitiesRef);
    FluidMath.Densities = GraphBuilder.CreateUAV(DensitiesRef);
    FluidMath.SpatialIndices = GraphBuilder.CreateUAV(SpatialIndicesRef);
    FluidMath.SpatialOffsets = GraphBuilder.CreateUAV(SpatialOffsetsRef);

    FluidMath.CollisionDampening    = SimulationSettings.CollisionDampening;
    FluidMath.DeltaTime             = SimulationSettings.DeltaTime;
    FluidMath.Gravity               = SimulationSettings.Gravity;
    FluidMath.NumParticles          = SimulationSettings.NumParticles;
    FluidMath.PressureAmplifier     = SimulationSettings.PressureAmplifier;
    FluidMath.SmoothingRadius       = SimulationSettings.SmoothingRadius;
    FluidMath.TargetDensity         = SimulationSettings.TargetDensity;
    FluidMath.ViscosityStrength     = SimulationSettings.ViscosityStrength;

    FluidMath.FluidBounds = UBFluidBounds;

    FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, FluidMath);
    FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, FluidMath);
    FluidMathDispatch::SortAndCalculateOffsets(GraphBuilder, GlobalShaderMap, FluidMath);
    FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, FluidMath);
    FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, FluidMath);
    FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, FluidMath);
    FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, FluidMath);
}

void UParticleBuffers::DispatchFluidRender(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FRDGTexture* SceneColor, const FSceneView& InView)
{
    //Update UBs
    FFluidEnvironment FluidEnvironment;

    FluidEnvironment.ExtinctionCoeff = FVector3f(ParentVolume->ExtinctionCoeff);
    FluidEnvironment.MarchStepSize = ParentVolume->MarchStepSize;
    FluidEnvironment.LightStepSize = ParentVolume->LightStepSize;
    FluidEnvironment.DensityStepSize = ParentVolume->DensityStepSize;
    FluidEnvironment.DensityMultiplier = ParentVolume->DensityMultiplier;
    FluidEnvironment.indexOfRefraction = ParentVolume->indexOfRefraction;
    FluidEnvironment.NumRefractions = ParentVolume->NumRefraction;

    FTransform Cube(ParentVolume->Bounds->GetComponentRotation(), ParentVolume->Bounds->GetComponentLocation(), ParentVolume->Bounds->GetComponentScale());
    FluidEnvironment.CubeLocalToWorld = FMatrix44f(Cube.ToMatrixWithScale());
    FluidEnvironment.CubeWorldToLocal = FMatrix44f(Cube.ToMatrixWithScale().Inverse());

    UBFluidEnvironment = TUniformBufferRef<FFluidEnvironment>::CreateUniformBufferImmediate(FluidEnvironment, EUniformBufferUsage::UniformBuffer_SingleFrame);  
        
    // Render Prep
    FRenderPrepParams RenderPrep;
    
    RenderPrep.Positions = GraphBuilder.CreateSRV(PositionsRef);
    RenderPrep.PredictedPositions = GraphBuilder.CreateSRV(PredictedPositionsRef);
    RenderPrep.SpatialIndices = GraphBuilder.CreateSRV(SpatialIndicesRef);
    RenderPrep.SpatialOffsets = GraphBuilder.CreateSRV(SpatialOffsetsRef);
    RenderPrep.NumParticles = SimulationSettings.NumParticles;
    
    RenderPrep.FluidBounds = UBFluidBounds;
    RenderPrep.FluidVolume = UBFluidVolume;

    FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(DensityMap);
    RenderPrep.DensityMap = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DensityMapRef));
    RenderPrep.DensityMapSize = FUintVector3(DensityMap->GetDesc().GetSize());

    RenderDispatch::GenerateDensityMap(GraphBuilder, GlobalShaderMap, RenderPrep);

    // Render
    FFluidMarchParams FluidParams;
    
    FRDGTextureDesc OutputDesc {};
    OutputDesc = SceneColor->Desc;
    //OutputDesc.Extent /= 4.0;
    OutputDesc.Reset();
    OutputDesc.Flags |= TexCreate_UAV;
    OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
    const FLinearColor ClearColor(0., 0., 0., 0.);
    OutputDesc.ClearValue = FClearValueBinding(ClearColor);
    const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("Atlas Output"));
    FluidParams.Target = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));

    FluidParams.DensityMap = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(DensityMapRef));
    FluidParams.DensityMapSize = FUintVector3(DensityMap->GetDesc().GetSize());
    FluidParams.FluidVolume = UBFluidVolume;
    FluidParams.FluidBounds = UBFluidBounds;
    FluidParams.Enviroment = UBFluidEnvironment;
    FluidParams.SceneColor = SceneColor;
    FluidParams.View = InView.ViewUniformBuffer;
    
    RenderDispatch::Raymarch(GraphBuilder, GlobalShaderMap, FluidParams);
    AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor);
}