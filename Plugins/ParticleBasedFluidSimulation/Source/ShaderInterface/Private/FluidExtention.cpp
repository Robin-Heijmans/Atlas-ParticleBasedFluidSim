#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "PostProcess/PostProcessInputs.h"
#include "Misc/Optional.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"	
#include "RHICommandList.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UnifiedBuffer.h"

#include "FluidBoundingVolume.h"
#include "ComputeLibrary.h"

namespace 
{
	TAutoConsoleVariable<int32> CVarShaderOn(
		TEXT("r.Fluid"),
		0,
		TEXT("Enable Fluid Rendering \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
}

FFluidExtention::FFluidExtention(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) 
{
	UE_LOG(LogTemp, Log, TEXT("Fluid: Custom SceneViewExtension registered"));

    // Default Params for now
    FluidVolume.BoundsPosition = FVector3f(-288.779695,8.043881,122.825553);
    FluidVolume.BoundsSize = FVector3f(56,56,56);

    FluidBoundsLocal.MinBounds = FVector3f(-32,-32,-32);
    FluidBoundsLocal.MaxBounds = FVector3f(32,32,32);

    ENQUEUE_RENDER_COMMAND(GenDensityMap)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        // Particle Buffer
        FRDGBufferDesc PositionsDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), 500);
        FRDGBufferDesc PredictedPositionsDesc   = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), 500);
        FRDGBufferDesc VelocitiesDesc           = FRDGBufferDesc::CreateStructuredDesc(sizeof(FVector3f), 500);
        FRDGBufferDesc DensitiesDesc            = FRDGBufferDesc::CreateStructuredDesc(sizeof(float), 500);
        FRDGBufferDesc SpatialIndicesDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(FUintVector3), 500);
        FRDGBufferDesc SpatialOffsetsDesc       = FRDGBufferDesc::CreateStructuredDesc(sizeof(uint32), 500);

        FRDGBufferRef Positions          = GraphBuilder.CreateBuffer(PositionsDesc,             TEXT("TanFluid Positions"));
        FRDGBufferRef PredictedPositions = GraphBuilder.CreateBuffer(PredictedPositionsDesc,    TEXT("TanFluid PredictedPositions"));
        FRDGBufferRef Velocities         = GraphBuilder.CreateBuffer(VelocitiesDesc,            TEXT("TanFluid Velocities"));
        FRDGBufferRef Densities          = GraphBuilder.CreateBuffer(DensitiesDesc,             TEXT("TanFluid Densities"));
        FRDGBufferRef SpatialIndices     = GraphBuilder.CreateBuffer(SpatialIndicesDesc,        TEXT("TanFluid SpatialIndices"));
        FRDGBufferRef SpatialOffsets     = GraphBuilder.CreateBuffer(SpatialOffsetsDesc,        TEXT("TanFluid SpatialOffsets"));

        ParticleBuffers.Positions             = GraphBuilder.ConvertToExternalBuffer(Positions         );
        ParticleBuffers.PredictedPositions    = GraphBuilder.ConvertToExternalBuffer(PredictedPositions);
        ParticleBuffers.Velocities            = GraphBuilder.ConvertToExternalBuffer(Velocities        );
        ParticleBuffers.Densities             = GraphBuilder.ConvertToExternalBuffer(Densities         );
        ParticleBuffers.SpatialIndices        = GraphBuilder.ConvertToExternalBuffer(SpatialIndices    );
        ParticleBuffers.SpatialOffsets        = GraphBuilder.ConvertToExternalBuffer(SpatialOffsets    );

        const FRDGBufferRef PositionsRef            = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Positions         ,   TEXT("TanFluid Positions"));
        const FRDGBufferRef PredictedPositionsRef   = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.PredictedPositions,   TEXT("TanFluid PredictedPositions"));
        const FRDGBufferRef VelocitiesRef           = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Velocities        ,   TEXT("TanFluid Velocities"));
        const FRDGBufferRef DensitiesRef            = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Densities         ,   TEXT("TanFluid Densities"));
        const FRDGBufferRef SpatialIndicesRef       = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.SpatialIndices    ,   TEXT("TanFluid SpatialIndices"));
        const FRDGBufferRef SpatialOffsetsRef       = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.SpatialOffsets    ,   TEXT("TanFluid SpatialOffsets"));
        
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

        //Density Map UwU
        FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("DensityMap"))
                .SetExtent(128, 128)
                .SetDepth(128)
                .SetFormat(PF_A32B32G32R32F)
                .SetFlags(ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource)
                .SetInitialState(ERHIAccess::SRVCompute);

        DensityMap = RHICreateTexture(Desc);
        
        const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluid DensityMap")));
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, PositionsRef);

        //GraphBuilder.QueueBufferExtraction(ParticlePositionsRef, &PooledPositions);
        GraphBuilder.Execute();
    });
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) 
{
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

    ENQUEUE_RENDER_COMMAND(GenDensityMap)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);
            
        const FRDGBufferRef PositionsRef            = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Positions         ,   TEXT("TanFluid Positions"));
        const FRDGBufferRef PredictedPositionsRef   = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.PredictedPositions,   TEXT("TanFluid PredictedPositions"));
        const FRDGBufferRef VelocitiesRef           = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Velocities        ,   TEXT("TanFluid Velocities"));
        const FRDGBufferRef DensitiesRef            = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Densities         ,   TEXT("TanFluid Densities"));
        const FRDGBufferRef SpatialIndicesRef       = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.SpatialIndices    ,   TEXT("TanFluid SpatialIndices"));
        const FRDGBufferRef SpatialOffsetsRef       = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.SpatialOffsets    ,   TEXT("TanFluid SpatialOffsets"));
        
        FluidMath.Positions = GraphBuilder.CreateUAV(PositionsRef);
        FluidMath.PredictedPositions = GraphBuilder.CreateUAV(PredictedPositionsRef);
        FluidMath.Velocities = GraphBuilder.CreateUAV(VelocitiesRef);
        FluidMath.Densities = GraphBuilder.CreateUAV(DensitiesRef);
        FluidMath.SpatialIndices = GraphBuilder.CreateUAV(SpatialIndicesRef);
        FluidMath.SpatialOffsets = GraphBuilder.CreateUAV(SpatialOffsetsRef);

        FluidMath.CollisionDampening = 0.6f;
        FluidMath.DeltaTime = 1.f/60.f;
        FluidMath.Gravity = -98.1f;
        FluidMath.NumParticles = 500;
        FluidMath.PressureAmplifier = 100.f;
        FluidMath.SmoothingRadius = 4.f;
        FluidMath.TargetDensity = 3.f;
        FluidMath.ViscosityStrength = 1.f;

        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, FluidMath, FluidBoundsLocal);
        FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, FluidMath, FluidBoundsLocal);
        FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, FluidMath,FluidBoundsLocal);
        FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, FluidMath, FluidBoundsLocal);
        FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, FluidMath, FluidBoundsLocal);
        FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, FluidMath, FluidBoundsLocal);

        GraphBuilder.Execute();
    });
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) 
{

	// Dipatch Shader here
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;

    // Get ShaderMap
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    // Update Uniform Buffers
    // Particles

    // -- General Pipeline --
    // 1. Physics Simulation
    // 2. Generate Density Map / Render Prep
    // 3. Dispatch Fluid March / Rendering

    // Physics Simulation
    //ParticleSimulation.Dispatch(GraphBuilder, GlobalShaderMap, Particles);

    // Render Prep
    const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluid DensityMap")));
    const FRDGBufferRef PositionsRef = GraphBuilder.RegisterExternalBuffer(ParticleBuffers.Positions, TEXT("TanFluid Positions"));
        
    RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, PositionsRef);

    // Fluid March
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor, FluidVolume, DensityMapRef);

}