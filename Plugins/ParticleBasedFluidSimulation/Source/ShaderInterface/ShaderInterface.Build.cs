// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ShaderInterface : ModuleRules
{
	public ShaderInterface(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		var EngineDir = Path.GetFullPath(Target.RelativeEnginePath);

		PrivateIncludePaths.AddRange(
			new string[] {
				// Required to find PostProcessing includes f.ex. screenpass.h & TranslucentPassResource.h
				Path.Combine(EngineDir, "Source/Runtime/Renderer/Private"),
				Path.Combine(EngineDir, "Source/Runtime/Renderer/Internal")
			});
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
            	"Core",
            	"CoreUObject",
            	"Engine",
            	"RenderCore",
            	"Renderer",
            	"RHI",
            	"Projects",
				"MaterialShaderQualitySettings"
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
            	"Renderer",
            	"RenderCore",
            	"RHI",
            	"Projects"
				// ... add private dependencies that you statically link with here ...	
			}
			);
	}
}
