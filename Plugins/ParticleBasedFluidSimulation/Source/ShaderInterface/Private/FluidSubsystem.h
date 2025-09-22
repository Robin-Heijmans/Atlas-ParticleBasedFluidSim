#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "FluidSubsystem.generated.h"

UCLASS()
class UFluidSubsystem : public UEngineSubsystem {
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	TSharedPtr<class FFluidExtention, ESPMode::ThreadSafe> FluidExtention;
};