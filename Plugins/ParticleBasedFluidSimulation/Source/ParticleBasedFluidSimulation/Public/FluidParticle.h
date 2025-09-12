#pragma once

#include "CoreMinimal.h"

#include "FluidParticle.generated.h"
USTRUCT(BlueprintType)
struct FParticle {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector PredictedPosition = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Mass = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Density = 0.0f;
};