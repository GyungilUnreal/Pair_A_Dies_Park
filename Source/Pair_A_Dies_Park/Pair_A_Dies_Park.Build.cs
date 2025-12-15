// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Pair_A_Dies_Park : ModuleRules
{
	public Pair_A_Dies_Park(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", 
			"GameplayAbilities", "GameplayTags", "GameplayTasks", 
			"UMG", "SlateCore",
			"OnlineSubsystem", "OnlineSubsystemUtils",
            "LevelSequence", "MovieScene", "Steamworks"
        });

        DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
        DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");
    }
}
