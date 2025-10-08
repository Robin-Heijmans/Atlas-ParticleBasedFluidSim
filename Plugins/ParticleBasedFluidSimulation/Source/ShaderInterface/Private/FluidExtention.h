#pragma once

#include "CoreMinimal.h"
#include "RenderGraphUtils.h"
#include "SceneViewExtension.h"
#include "SceneRendererInterface.h"
#include "DataDrivenShaderPlatformInfo.h"
#include "PostProcess/PostProcessMaterial.h"
#include "SceneView.h"

#include "ParticleLibrary.h"
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
	virtual void PreRenderView_RenderThread(FRDGBuilder& GraphBuilder, FSceneView& InViewy) override;
	virtual void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& InView, const FPostProcessingInputs& Inputs) override;

private:
	// Shader 'Dispatchers'
	FRenderPrepDispatchParams RenderPrep;
	FFluidMarchDispatchParams FluidMarch;
	FFluidMathParams FluidMath;

	TRefCountPtr<IPooledRenderTarget> DensityMap;
	
	// Uniform Buffers
	TUniformBufferRef<FFluidEnvironment> UBFluidEnvironment;
	TUniformBufferRef<FFluidVolumeLocal> UBFluidBounds;
	TUniformBufferRef<FFluidVolume> UBFluidVolume;

	float TotalTime = 0.0f;
    const float FixedTimeStep = 1.f / 60.f;
};