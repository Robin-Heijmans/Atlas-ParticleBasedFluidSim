#pragma once

#include "CoreMinimal.h"
#include "SpatialLookup.generated.h"
USTRUCT()
struct FSpatialLookupEntry {
    GENERATED_BODY()

    uint32 Key;
    int ParticleIndex;
};

FORCEINLINE bool operator<(const FSpatialLookupEntry& A, const FSpatialLookupEntry& B)
{
    return A.Key < B.Key;
}

static const TStaticArray<FIntVector, 27> Offsets3D =
{
	FIntVector(-1, -1, -1),
	FIntVector(-1, -1, 0),
	FIntVector(-1, -1, 1),
	FIntVector(-1, 0, -1),
	FIntVector(-1, 0, 0),
	FIntVector(-1, 0, 1),
	FIntVector(-1, 1, -1),
	FIntVector(-1, 1, 0),
	FIntVector(-1, 1, 1),
	FIntVector(0, -1, -1),
	FIntVector(0, -1, 0),
	FIntVector(0, -1, 1),
	FIntVector(0, 0, -1),
	FIntVector(0, 0, 0),
	FIntVector(0, 0, 1),
	FIntVector(0, 1, -1),
	FIntVector(0, 1, 0),
	FIntVector(0, 1, 1),
	FIntVector(1, -1, -1),
	FIntVector(1, -1, 0),
	FIntVector(1, -1, 1),
	FIntVector(1, 0, -1),
	FIntVector(1, 0, 0),
	FIntVector(1, 0, 1),
	FIntVector(1, 1, -1),
	FIntVector(1, 1, 0),
	FIntVector(1, 1, 1)
};