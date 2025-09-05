#pragma once
    
#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class fluid_simulationCore : public IModuleInterface
{
public:
    static inline fluid_simulationCore& Get()
    {
        return FModuleManager::LoadModuleChecked<fluid_simulationCore>("fluid_simulationCore");
    }

    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("fluid_simulationCore");
    }

    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};