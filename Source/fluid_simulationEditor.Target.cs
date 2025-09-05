using UnrealBuildTool;

public class fluid_simulationEditorTarget : TargetRules
{
    public fluid_simulationEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.Latest;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        CppStandard = CppStandardVersion.Latest;
        ExtraModuleNames.AddRange( new string[] { "fluid_simulationCore" } );
    }
}