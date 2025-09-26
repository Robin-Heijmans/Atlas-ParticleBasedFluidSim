// Example from: https://unreal.shadeup.dev/docs/compute (08.09.2025)

#pragma once

#include "CoreMinimal.h"
#include "Shader.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "Components/ActorComponent.h"	
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

	void Initialize(uint32 NumParticles, FFluidVolumeLocal VolumeBounds);
	
	// Call this at thw beginning of the frame
	void Register(FRDGBuilder& GraphBuilder);

	FFluidMathParams GetParticleParameters(FRDGBuilder& GraphBuilder);
	FRDGBufferSRVRef GetRenderPrepParameters(FRDGBuilder& GraphBuilder);

	uint32 NumParticles = 0;
	bool bInitialized = false;
private:
	void CreateUAVs( 
		FRDGBuilder& GraphBuilder,
		FRDGBufferUAV*& OutPositions, 
		FRDGBufferUAV*& OutPredictedPositions, 
		FRDGBufferUAV*& OutVelocities, 
		FRDGBufferUAV*& OutDensities, 
		FRDGBufferUAV*& OutSpatialIndcies, 
		FRDGBufferUAV*& OutSpatialOffsets
	);

	void CreateSRVs(
		FRDGBuilder& GraphBuilder,
		FRDGBufferSRV*& OutPositions
	);

	// Persistent through frames
	TRefCountPtr<FRDGPooledBuffer> Positions;
	TRefCountPtr<FRDGPooledBuffer> PredictedPositions;
	TRefCountPtr<FRDGPooledBuffer> Velocities;
	TRefCountPtr<FRDGPooledBuffer> Densities;
	TRefCountPtr<FRDGPooledBuffer> SpatialIndices;
	TRefCountPtr<FRDGPooledBuffer> SpatialOffsets;

	// These are local to a single frame
	FRDGBufferRef PositionsRef = nullptr;
	FRDGBufferRef PredictedPositionsRef = nullptr;
	FRDGBufferRef VelocitiesRef = nullptr;
	FRDGBufferRef DensitiesRef = nullptr;
	FRDGBufferRef SpatialIndicesRef = nullptr;
	FRDGBufferRef SpatialOffsetsRef = nullptr;

	FFluidVolumeLocal FluidBoundsLocal;
};