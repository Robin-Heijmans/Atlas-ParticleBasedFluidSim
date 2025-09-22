#pragma once

#include "CoreMinimal.h"
#include "RenderGraphUtils.h"
#include "SceneViewExtension.h"
#include "SceneRendererInterface.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "PostProcess/PostProcessMaterial.h"
#include "SceneView.h"

#include "Shaders.h"

class FFluidExtention : public FSceneViewExtensionBase 
{
public:
	FFluidExtention(const FAutoRegister& AutoRegister);

	virtual int32 GetPriority() const { return 1 << 16; };
	virtual void SetupViewFamily(FSceneViewFamily& InViewFamily) override {};
	virtual void SetupView(FSceneViewFamily& InViewFamily, FSceneView& InView) override {};


	/* Setup before rendering happens in here. */
	virtual void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override;

	/* All the rendering happens in here. */
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;

private:
	// Shader 'Dispatchers'
	FParticleSimulationDispatchParams ParticleSimulation;
	FRenderPrepDispatchParams RenderPrep;
	FFluidMarchDispatchParams FluidMarch;

	FRDGBufferRef RWBuffer;
	TRefCountPtr<FRDGPooledBuffer> PooledBuffer;

	// Uniform Buffers
	FFluidVolume FluidVolume;
};