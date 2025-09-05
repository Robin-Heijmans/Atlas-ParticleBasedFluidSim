#include "fluid_simulationCore.h"
#include "Modules/ModuleManager.h"

#include "Log.h"

void fluid_simulationCore::StartupModule()
{
    UE_LOG(Logfluid_simulationCore, Log, TEXT("fluid_simulationCore module starting up"));
}

void fluid_simulationCore::ShutdownModule()
{
    UE_LOG(Logfluid_simulationCore, Log, TEXT("fluid_simulationCore module shutting down"));
}

IMPLEMENT_PRIMARY_GAME_MODULE(fluid_simulationCore, fluid_simulationCore, "fluid_simulationCore");
