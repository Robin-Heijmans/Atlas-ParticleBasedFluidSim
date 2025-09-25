// Example from: https://unreal.shadeup.dev/docs/compute (08.09.2025)

#pragma once

#include "CoreMinimal.h"
#include "Shader.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"

#include "Shaders.h"
#include "ParticleLibrary.generated.h"



UCLASS()
class SHADERINTERFACE_API UParticleBuffers : public UObject
{
GENERATED_BODY()
public:
	UParticleBuffers() { UE_LOG(LogTemp, Error, TEXT("Atlas: cannot use default constructor for UParticleBuffers... I know it sucks")); }
	UParticleBuffers(const uint32 NumParticles, const FFluidVolumeLocal& VolumeBounds);
	~UParticleBuffers();

	// Call this at thw beginning of the frame
	void Register(FRDGBuilder& GraphBuilder);

	FFluidMathParams GetParticleParameters(FRDGBuilder& GraphBuilder);

private:
	void CreateUAVs( 
		FRDGBuilder& GraphBuilder,
		FRDGBufferUAV* OutPositions = nullptr, 
		FRDGBufferUAV* OutPredictedPositions = nullptr, 
		FRDGBufferUAV* OutVelocities = nullptr, 
		FRDGBufferUAV* OutDensities = nullptr, 
		FRDGBufferUAV* OutSpatialIndcies = nullptr, 
		FRDGBufferUAV* OutSpatialOffsets = nullptr
	);

	void CreateSRVs(
		FRDGBuilder& GraphBuilder,
		FRDGBufferSRV* OutPositions = nullptr, 
		FRDGBufferSRV* OutPredictedPositions = nullptr, 
		FRDGBufferSRV* OutVelocities = nullptr, 
		FRDGBufferSRV* OutDensities = nullptr, 
		FRDGBufferSRV* OutSpatialIndcies = nullptr, 
		FRDGBufferSRV* OutSpatialOffsets = nullptr
	);

	TRefCountPtr<FRDGPooledBuffer> Positions;
	TRefCountPtr<FRDGPooledBuffer> PredictedPositions;
	TRefCountPtr<FRDGPooledBuffer> Velocities;
	TRefCountPtr<FRDGPooledBuffer> Densities;
	TRefCountPtr<FRDGPooledBuffer> SpatialIndices;
	TRefCountPtr<FRDGPooledBuffer> SpatialOffsets;

	FRDGBufferRef PositionsRef;    
	FRDGBufferRef PredictedPositionsRef;
	FRDGBufferRef VelocitiesRef;
	FRDGBufferRef DensitiesRef;
	FRDGBufferRef SpatialIndicesRef;
	FRDGBufferRef SpatialOffsetsRef;

	FFluidVolumeLocal FluidBoundsLocal;
};