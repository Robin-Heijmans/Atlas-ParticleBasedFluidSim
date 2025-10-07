#include "FluidExtention.h"

#include "CoreMinimal.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "SceneView.h"
#include "PostProcess/PostProcessInputs.h"
#include "Misc/Optional.h"
#include "GameFramework/Actor.h"
#include "RHICommandList.h"

#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "UnifiedBuffer.h"	
#include "GameFramework/Actor.h"
#include "UObject/UObjectGlobals.h"

#include "FluidBoundingVolume.h"

constexpr uint32 DensityMapSize(256);

namespace 
{
	TAutoConsoleVariable<int32> CVarRendering(
		TEXT("Atlas.Rendering"),
		0,
		TEXT("Enable Fluid Rendering \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
        
	TAutoConsoleVariable<int32> CVarSimulation(
		TEXT("Atlas.Simulation"),
		0,
		TEXT("Enable Fluid Simulation \n")
		TEXT(" 0: OFF;")
		TEXT(" 1: ON."),
		ECVF_RenderThreadSafe);
}

FFluidExtention::FFluidExtention(const FAutoRegister& AutoRegister) : FSceneViewExtensionBase(AutoRegister) 
{
	UE_LOG(LogTemp, Log, TEXT("Fluid: Custom SceneViewExtension registered"));

    // Init Density Map
    ENQUEUE_RENDER_COMMAND(GenDensityMap)(
    [this](FRHICommandListImmediate& RHICmdList) {
        FRDGBuilder GraphBuilder(RHICmdList);

        //Density Map UwU
        FRHITextureCreateDesc Desc = FRHITextureCreateDesc::Create3D(TEXT("Atlas DensityMap"))
                .SetExtent(DensityMapSize, DensityMapSize)
                .SetDepth(DensityMapSize)
                .SetFormat(PF_R32_FLOAT)
                .SetFlags(ETextureCreateFlags::UAV | ETextureCreateFlags::ShaderResource)
                .SetInitialState(ERHIAccess::SRVCompute);

        DensityMap = RHICreateTexture(Desc);
        
        const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("Atlas DensityMap")));
        FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
        GraphBuilder.Execute();
    });
}

void FFluidExtention::BeginRenderViewFamily(FSceneViewFamily& ViewFamily) 
{
    // Check for new volumes, that need particles
    UWorld* World = ViewFamily.Scene->GetWorld(); 
    if(World == nullptr) return;

    // Advance simulation
    #if WITH_EDITOR
        if(World->IsPlayInEditor() && CVarSimulation.GetValueOnRenderThread() == 1) 
        {
            TotalTime += World->GetDeltaSeconds();
        }
    #else
        TotalTime += World->GetDeltaSeconds();
    #endif

    FFluidVolume FluidVolume;
    FFluidVolumeLocal VolumeBounds;
    FFluidEnvironment FluidEnvironment;

    for (TActorIterator<AFluidBoundingVolume> FluidVolumes(World); FluidVolumes; ++FluidVolumes)
    {
        // Update UBOs continously
        TArray<FVector> bounds = FluidVolumes->GetVolumeBounds();
        VolumeBounds.MinBounds = FVector3f(bounds[0]);
        VolumeBounds.MaxBounds = FVector3f(bounds[1]);
        
        FluidVolume.BoundsPosition = FVector3f(FluidVolumes->Bounds->GetComponentLocation());
        FluidVolume.BoundsSize = FVector3f(FluidVolumes->Bounds->GetScaledBoxExtent());
        
        FTransform Cube(FluidVolumes->Bounds->GetComponentRotation(), FluidVolumes->Bounds->GetComponentLocation(), FluidVolumes->Bounds->GetComponentScale());

        FluidEnvironment.CubeLocalToWorld = FMatrix44f(Cube.ToMatrixWithScale());
        FluidEnvironment.CubeWorldToLocal = FMatrix44f(Cube.ToMatrixWithScale().Inverse());

        FluidEnvironment.ExtinctionCoeff = FVector3f(FluidVolumes->ExtinctionCoeff);
        FluidEnvironment.MarchStepSize = FluidVolumes->MarchStepSize;
        FluidEnvironment.LightStepSize = FluidVolumes->LightStepSize;
        FluidEnvironment.DensityStepSize = FluidVolumes->DensityStepSize;
        FluidEnvironment.DensityMultiplier = FluidVolumes->DensityMultiplier;
        FluidEnvironment.indexOfRefraction = FluidVolumes->indexOfRefraction;
        FluidEnvironment.NumRefractions = FluidVolumes->NumRefraction;

        UBFluidEnvironment = TUniformBufferRef<FFluidEnvironment>::CreateUniformBufferImmediate(FluidEnvironment, EUniformBufferUsage::UniformBuffer_SingleFrame);  
        UBFluidBounds = TUniformBufferRef<FFluidVolumeLocal>::CreateUniformBufferImmediate(VolumeBounds, EUniformBufferUsage::UniformBuffer_SingleFrame);  
        UBFluidVolume = TUniformBufferRef<FFluidVolume>::CreateUniformBufferImmediate(FluidVolume, EUniformBufferUsage::UniformBuffer_SingleFrame);  
    
        // Initialize ParticleBuffers
        if(!FluidVolumes->HasParticles)
        {   
            TArray<USceneComponent*> Children;
            FluidVolumes->GetRootComponent()->GetChildrenComponents(true, Children);
            for(USceneComponent* Child : Children)
            {
                UParticleBuffers* ParticleBuffers = nullptr;
                ParticleBuffers = dynamic_cast<UParticleBuffers*>(Child);
                if(ParticleBuffers && ParticleBuffers->bInitialized)
                {
                    ParticleBuffers->UnregisterComponent(); // doesnt working, but doesnt break anything either
                    
                }
            }

            const uint32 NumParticles = FluidVolumes->NumParticlesX * FluidVolumes->NumParticlesY * FluidVolumes->NumParticlesZ;
            if(NumParticles > 5000 || NumParticles == 0)     
            {
                UE_LOG(LogTemp, Warning, TEXT("Illegal NumParticles: %d"), NumParticles);
                continue;
            }

	        UParticleBuffers* ParticleBuffers = NewObject<UParticleBuffers>(*FluidVolumes,UParticleBuffers::StaticClass(), TEXT("Particle Buffers"));

            ParticleBuffers->RegisterComponent();
            ParticleBuffers->AttachToComponent(FluidVolumes->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

            // Allocate Particle Buffer
            ParticleBuffers->Initialize(UBFluidBounds, *FluidVolumes);
            ParticleBuffers->SimulationSettings.DeltaTime = FixedTimeStep;
            FluidVolumes->HasParticles = true;
            return;
        }
    }

    for (TObjectIterator<UParticleBuffers> ParticleBuffers; ParticleBuffers; ++ParticleBuffers)
    {
        if(!ParticleBuffers->bInitialized) continue;

        // Particle Simlation
        if(TotalTime > FixedTimeStep)
        {
            ENQUEUE_RENDER_COMMAND(ParticleSimulation)(
            [this, ParticleBuffers](FRHICommandListImmediate& RHICmdList) {
                FRDGBuilder GraphBuilder(RHICmdList);
                ParticleBuffers->Register(GraphBuilder);
                
                FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

                FFluidMathParams FluidMath = ParticleBuffers->GetParticleParameters(GraphBuilder);
                FluidMath.FluidBounds = UBFluidBounds;

                FluidMathDispatch::ExternalForces(GraphBuilder, GlobalShaderMap, FluidMath);
                FluidMathDispatch::UpdateSpatialLookup(GraphBuilder, GlobalShaderMap, FluidMath);
                FluidMathDispatch::SortAndCalculateOffsets(GraphBuilder, GlobalShaderMap, FluidMath);
                FluidMathDispatch::CalculateDensity(GraphBuilder, GlobalShaderMap, FluidMath);
                FluidMathDispatch::CalculatePressureForce(GraphBuilder, GlobalShaderMap, FluidMath);
                FluidMathDispatch::CalculateViscosityForce(GraphBuilder, GlobalShaderMap, FluidMath);
                FluidMathDispatch::UpdatePositions(GraphBuilder, GlobalShaderMap, FluidMath);

                GraphBuilder.Execute();
            });      
            TotalTime = 0.f;
        }

        // Density Map Generation
        if(CVarRendering.GetValueOnRenderThread())
        {
            ENQUEUE_RENDER_COMMAND(GenerateDensityMap)(
            [this, ParticleBuffers](FRHICommandListImmediate& RHICmdList) {
                FRDGBuilder GraphBuilder(RHICmdList);
                
                ParticleBuffers->Register(GraphBuilder);
                FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);

                // Render Prep
                FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("Atlas DensityMap")));

                FRenderPrepParams RenderPrepParams = ParticleBuffers->GetRenderPrepParameters(GraphBuilder);
                RenderPrepParams.FluidBounds = UBFluidBounds;
                RenderPrepParams.FluidVolume = UBFluidVolume;

                RenderPrepParams.DensityMap = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(DensityMapRef));
                RenderPrepParams.DensityMapSize = FUintVector3(DensityMapSize); // PLS PUT ME OUT OF MY MISERY

                RenderPrep.Dispatch(GraphBuilder, GlobalShaderMap, RenderPrepParams);
                    
                GraphBuilder.Execute();
            });
        }
    }
}

void FFluidExtention::PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) 
{

	// Dipatch Shader here
    if (CVarRendering.GetValueOnRenderThread() == 0) return; 


    // Get ShaderMap
    FGlobalShaderMap* GlobalShaderMap = GetGlobalShaderMap(InView.Family->GetFeatureLevel());

    
    const FRDGTextureRef DensityMapRef = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(DensityMap, TEXT("Atlas DensityMap")));
        
    // Get SceneColor
	const FSceneViewFamily& ViewFamily = *InView.Family;    
	FRDGTexture* SceneColor = Inputs.SceneTextures->GetContents()->SceneColorTexture;

    // Output Texture
    FRDGTextureDesc OutputDesc {};
    OutputDesc = SceneColor->Desc;
    //OutputDesc.Extent /= 4.0;
    OutputDesc.Reset();
    OutputDesc.Flags |= TexCreate_UAV;
    OutputDesc.Flags &= ~(TexCreate_RenderTargetable | TexCreate_FastVRAM);
    const FLinearColor ClearColor(0., 0., 0., 0.);
    OutputDesc.ClearValue = FClearValueBinding(ClearColor);
    const FRDGTextureRef OutputTexture = GraphBuilder.CreateTexture(OutputDesc, TEXT("Atlas Output"));
    
    FFluidMarchParams FluidParams;
    FluidParams.Target = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(OutputTexture));
    FluidParams.DensityMap = GraphBuilder.CreateSRV(FRDGTextureSRVDesc(DensityMapRef));
    FluidParams.DensityMapSize = FUintVector3(256);
    FluidParams.FluidVolume = UBFluidVolume;
    FluidParams.FluidBounds = UBFluidBounds;
    FluidParams.Enviroment = UBFluidEnvironment;
    FluidParams.SceneColor = SceneColor;
    FluidParams.View = InView.ViewUniformBuffer;
    
    FluidMarch.Dispatch(GraphBuilder, GlobalShaderMap, FluidParams);
    AddCopyTexturePass(GraphBuilder, OutputTexture, SceneColor);

}