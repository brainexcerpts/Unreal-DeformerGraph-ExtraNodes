// Copyright Epic Games, Inc. All Rights Reserved.

#include "DeformerGraphBonusTools.h"

#include "Interfaces/IPluginManager.h"


#define LOCTEXT_NAMESPACE "FDeformerGraphBonusToolsModule"

void FDeformerGraphBonusToolsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; 
    // the exact timing is specified in the .uplugin file per-module
    
    // We register the folder in which we store the shader files of our new nodes:
	FString PluginShaderDir = IPluginManager::Get().FindPlugin(TEXT("DeformerGraphBonusTools"))->GetBaseDir();
	PluginShaderDir = FPaths::Combine(PluginShaderDir, TEXT("Source"));
	PluginShaderDir = FPaths::Combine(PluginShaderDir, TEXT("DeformerGraphBonusTools"));
	PluginShaderDir = FPaths::Combine(PluginShaderDir, TEXT("Shader"));	

	AddShaderSourceDirectoryMapping(TEXT("/Plugin/DeformerGraphBonusTools"), PluginShaderDir);
}

void FDeformerGraphBonusToolsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  
    // For modules that support dynamic reloading,
	// we call this function before unloading the module.	
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDeformerGraphBonusToolsModule, DeformerGraphBonusTools)