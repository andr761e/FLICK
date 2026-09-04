// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class FLICK : ModuleRules
{
	public FLICK(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.Add(ModuleDirectory);
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "PhysicsCore", "OnlineSubsystem", "OnlineSubsystemUtils", "HTTP", "Json" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "MoviePlayer", "RenderCore" });
		
	}
}
