using UnrealBuildTool;
    
public class fluid_simulationTarget : TargetRules
{
    public fluid_simulationTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        CppStandard = CppStandardVersion.Latest;
        ExtraModuleNames.AddRange( new string[] { "fluid_simulationCore" } );
    }
}