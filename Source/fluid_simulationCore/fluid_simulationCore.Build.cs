using UnrealBuildTool;

public class fluid_simulationCore : ModuleRules
{
    public fluid_simulationCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ParticleBasedFluidSimulation" });
        PrivateDependencyModuleNames.AddRange(new string[] { });
    }
}