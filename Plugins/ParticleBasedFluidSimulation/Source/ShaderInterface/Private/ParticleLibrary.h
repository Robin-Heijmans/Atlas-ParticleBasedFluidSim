// Example from: https://unreal.shadeup.dev/docs/compute (08.09.2025)

#pragma once

#include "CoreMinimal.h"
#include "Shader.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "Components/SceneComponent.h"

#include "Shaders.h"
#include "ParticleLibrary.generated.h"



UCLASS()
class SHADERINTERFACE_API UParticleBuffers : public USceneComponent
{
GENERATED_BODY()
public:
	UParticleBuffers() = default;
	~UParticleBuffers();

	void Initialize(class UFluidBoundingVolumeComponent* Volume);
	
	// Call this at the beginning of the frame
	void Register(FRDGBuilder& GraphBuilder);
	virtual void OnUnregister() override;

	void DispatchFluidMath(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap);
	void DispatchPOCollisionResolution(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FFluidMathParams& FluidMath);
	void DispatchFluidRender(FRDGBuilder& GraphBuilder, FGlobalShaderMap* GlobalShaderMap, FRDGTexture* SceneColor, const FSceneView& InView);

	struct FSimulationSettings
	{
    	float CollisionDampening = 0.6f;
    	float DeltaTime = 1.f/60.f;
    	FVector Gravity = FVector(0, 0, -98.1f);
    	float PressureAmplifier = 100.f;
    	float SmoothingRadius = 3.f;
    	float TargetDensity = 5.f;
    	float ViscosityStrength = 1.f;
    	uint32 NumParticles = 0;
	} SimulationSettings;
	bool bInitialized = false;
private:

	// Persistent through frames
	TRefCountPtr<FRDGPooledBuffer> Positions;
	TRefCountPtr<FRDGPooledBuffer> PredictedPositions;
	TRefCountPtr<FRDGPooledBuffer> Velocities;
	TRefCountPtr<FRDGPooledBuffer> Densities;
	TRefCountPtr<FRDGPooledBuffer> SpatialIndices;
	TRefCountPtr<FRDGPooledBuffer> SpatialOffsets;
	
	const FUintVector3 DensityMapSize = FUintVector3(128, 128, 128);
	TRefCountPtr<IPooledRenderTarget> DensityMap;

	// Uniform Buffers
	TUniformBufferRef<FFluidEnvironment> UBFluidEnvironment;
	TUniformBufferRef<FFluidVolumeLocal> UBFluidBounds;
	TUniformBufferRef<FFluidVolume> UBFluidVolume;

	// These are local to a single frame
	FRDGBufferRef PositionsRef = nullptr;
	FRDGBufferRef PredictedPositionsRef = nullptr;
	FRDGBufferRef VelocitiesRef = nullptr;
	FRDGBufferRef DensitiesRef = nullptr;
	FRDGBufferRef SpatialIndicesRef = nullptr;
	FRDGBufferRef SpatialOffsetsRef = nullptr;
	
	class UFluidBoundingVolumeComponent* ParentVolume;
};