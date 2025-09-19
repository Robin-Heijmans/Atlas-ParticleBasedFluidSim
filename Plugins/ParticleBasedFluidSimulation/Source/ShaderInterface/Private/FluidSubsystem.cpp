#include "FluidSubsystem.h"
#include "FluidExtention.h"
#include "SceneViewExtension.h"
#include "Misc/Optional.h"

void UFluidSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	FluidExtention = FSceneViewExtensions::NewExtension<FFluidExtention>();
	UE_LOG(LogTemp, Log, TEXT("Fluid: Subsystem initialized & SceneViewExtension created"));
}

void UFluidSubsystem::Deinitialize() {
	{
		FluidExtention->IsActiveThisFrameFunctions.Empty();

		FSceneViewExtensionIsActiveFunctor IsActiveFunctor;

		IsActiveFunctor.IsActiveFunction = [](const ISceneViewExtension* SceneViewExtension, const FSceneViewExtensionContext& Context)
		{
			return TOptional<bool>(false);
		};

		FluidExtention->IsActiveThisFrameFunctions.Add(IsActiveFunctor);
	}

	FluidExtention.Reset();
	FluidExtention = nullptr;
}