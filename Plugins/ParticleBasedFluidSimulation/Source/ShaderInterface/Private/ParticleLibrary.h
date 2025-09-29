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

	void Initialize(TUniformBufferRef<FFluidVolumeLocal> VolumeBounds, const class AFluidBoundingVolume* Volume);
	
	// Call this at thw beginning of the frame
	void Register(FRDGBuilder& GraphBuilder);

	FFluidMathParams GetParticleParameters(FRDGBuilder& GraphBuilder);
	// DensityMap left blanc
	FRenderPrepParams GetRenderPrepParameters(FRDGBuilder& GraphBuilder);

	struct FSimulationSettings
	{
    	float CollisionDampening = 0.6f;
    	float DeltaTime = 1.f/60.f;
    	float Gravity = -98.1f;
    	float PressureAmplifier = 100.f;
    	float SmoothingRadius = 4.f;
    	float TargetDensity = 3.f;
    	float ViscosityStrength = 1.f;
    	uint32 NumParticles = 0;
	} SimulationSettings;
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
		FRDGBufferSRV*& OutPositions,
    	FRDGBufferSRV*& OutPredictedPositions,
		FRDGBufferSRV*& OutSpatialIndices, 
		FRDGBufferSRV*& OutSpatialOffsets
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
	
	TArray<FVector3f> _positions;
	TArray<FVector3f> _preditctedpositions;
	TArray<FVector3f> _velocities;
	TArray<float> _densities;
	TArray<FUintVector3> _spatialindicies;
	TArray<uint32> _spatialoffsets;
};