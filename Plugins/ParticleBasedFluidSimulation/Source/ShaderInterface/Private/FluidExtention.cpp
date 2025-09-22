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

    ENQUEUE_RENDER_COMMAND(GenDensityMap)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        const uint32 NumElements = 500;
        const uint32 BytesPerElement = sizeof(FVector3f);

        // Position Buffer
        TArray<FVector3f> TmpPositions;
        TmpPositions.Init(FVector3f(1,1,1), NumElements);

        PooledPositions = GraphBuilder.ConvertToExternalBuffer(CreateStructuredBuffer(GraphBuilder, TEXT("TanFluidShader_ParticlePositions"), BytesPerElement, NumElements, TmpPositions.GetData(),TmpPositions.GetTypeSize() * TmpPositions.Num(), ERDGInitialDataFlags::None));
        const FRDGBufferRef ParticlePositionsRef = GraphBuilder.RegisterExternalBuffer(PooledPositions, TEXT("TanFluidShader_ParticlePositions"));
        //GraphBuilder.QueueBufferUpload(ParticlePositionsRef, TmpPositions.GetData(), TmpPositions.GetTypeSize() * TmpPositions.Num(), ERDGInitialDataFlags::None);

        //CreateStructuredBuffer(GraphBuilder, TEXT("TanFluidShader_ParticlePositions"), BytesPerElement, NumElements, TmpPositions.GetData(),TmpPositions.GetTypeSize() * TmpPositions.Num(), ERDGInitialDataFlags::None)

	    //FRDGBufferDesc Desc = FRDGBufferDesc::CreateStructuredDesc(BytesPerElement, NumElements);
        //CreateStructuredBuffer(ParticlePositions, TEXT("TanFluidShader_ParticlePositions"));

        //const FRDGBufferRef ParticlePositionsRef = GraphBuilder.CreateBuffer(Desc, TEXT("Particle Positions"));
        //GraphBuilder.QueueBufferUpload(ParticlePositionsRef, TmpPositions.GetData(), TmpPositions.GetTypeSize() * TmpPositions.Num(), ERDGInitialDataFlags::None);

        // Density Map
        FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("DensityMap"))
                .SetExtent(128, 128)
                .SetDepth(128)
                .SetFormat(PF_A32B32G32R32F)
                .SetFlags(ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource)
                .SetInitialState(ERHIAccess::SRVCompute);

        DensityMap = RHICreateTexture(Desc);

        const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluidShader_DensityMap")));
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, ParticlePositionsRef);

        //GraphBuilder.QueueBufferExtraction(ParticlePositionsRef, &PooledPositions);
        GraphBuilder.Execute();
    });
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) 
{
    if (CVarShaderOn.GetValueOnRenderThread() == 0) return; 

	UWorld* World = ViewFamily.Scene->GetWorld();
	if (World == nullptr) return;

	/* Fetch actors from the scene */
	TActorIterator<AFluidBoundingVolume> BoundingVolume(World);
	if (!BoundingVolume) return;

    FluidVolume.BoundsPosition = FVector3f(BoundingVolume->GetActorLocation());
    FluidVolume.BoundsSize = FVector3f(BoundingVolume->Bounds->GetScaledBoxExtent());
    
    static bool once = true;

    //if(once)
    {
        ENQUEUE_RENDER_COMMAND(GenDensityMap)(
        [this, BoundingVolume](FRHICommandListImmediate& RHICmdList) {
            FRDGBuilder GraphBuilder(RHICmdList);
            
            // Update Positions
            const uint32 NumElements = 500;
            const uint32 BytesPerElement = sizeof(FVector3f);

            TArray<FVector3f> Positions;
            Positions.Init(FVector3f::ZeroVector, NumElements);
            for(int i = 0; i < NumElements; i++)
            {
                Positions[i] = FVector3f(BoundingVolume->Particles[i].Position + BoundingVolume->Bounds->GetScaledBoxExtent()) / FVector3f(BoundingVolume->Bounds->GetScaledBoxExtent() * 2.f);
            }


            const FRDGBufferRef ParticlePositionsRef = GraphBuilder.RegisterExternalBuffer(PooledPositions, TEXT("TanFluidShader_ParticlePositions"));
            GraphBuilder.QueueBufferUpload(ParticlePositionsRef, Positions.GetData(), BytesPerElement * NumElements, ERDGInitialDataFlags::None);

            // Update Density Map
            const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluidShader_DensityMap")));
            FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
            RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef, ParticlePositionsRef);

            GraphBuilder.Execute();
        });
        once = false;
    }
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
    const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("TanFluidShader_DensityMap")));

    // -- General Pipeline --
    // 1. Physics Simulation
    // 2. Generate Density Map / Render Prep
    // 3. Dispatch Fluid March / Rendering

    // Physics Simulation
    //ParticleSimulation.Dispatch(GraphBuilder, GlobalShaderMap, Particles);

    // Render Prep
    //RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, DensityMapRef);

    // Fluid March
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, InView, SceneColor, FluidVolume, DensityMapRef);

}