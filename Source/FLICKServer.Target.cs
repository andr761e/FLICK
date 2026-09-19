// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class FLICKServerTarget : TargetRules
{
	public FLICKServerTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Server;
		if (Target.Platform == UnrealTargetPlatform.Mac)
		{
			DefaultBuildSettings = BuildSettingsVersion.Latest;
			IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		}
		else
		{
			DefaultBuildSettings = BuildSettingsVersion.V5;
			IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_6;
		}
		ExtraModuleNames.Add("FLICK");
	}
}
