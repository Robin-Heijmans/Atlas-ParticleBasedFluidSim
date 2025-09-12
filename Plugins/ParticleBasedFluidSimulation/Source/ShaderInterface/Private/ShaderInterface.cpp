// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShaderInterface.h"

#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "RHI.h"
#include "GlobalShader.h"
#include "RHICommandList.h"
#include "RenderGraphBuilder.h"
#include "RenderTargetPool.h"
#include "Runtime/Core/Public/Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FShaderInterfaceModule"

void FShaderInterfaceModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ParticleBasedFluidSimulation"))->GetBaseDir(), TEXT("Shaders/Compute/Private"));
	AddShaderSourceDirectoryMapping(TEXT("/Shaders"), PluginShaderDir);
	
	//TODO: add pixel shader paths
	//FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ParticleBasedFluidSimulation"))->GetBaseDir(), TEXT("Shaders/Compute/Private"));
	//AddShaderSourceDirectoryMapping(TEXT("/Shaders"), PluginShaderDir);
	//
	//FString PluginShaderDir = FPaths::Combine(IPluginManager::Get().FindPlugin(TEXT("ParticleBasedFluidSimulation"))->GetBaseDir(), TEXT("Shaders/Compute/Private"));
	//AddShaderSourceDirectoryMapping(TEXT("/Shaders"), PluginShaderDir);
}

void FShaderInterfaceModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FShaderInterfaceModule, ParticleBasedFluidSimulation)