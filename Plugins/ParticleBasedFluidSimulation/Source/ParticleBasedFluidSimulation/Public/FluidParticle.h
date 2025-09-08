#pragma once

#include "CoreMinimal.h"

#include "FluidParticle.generated.h"
USTRUCT(BlueprintType)
struct FParticle {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FVector Position = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Mass = 1.0f;
};