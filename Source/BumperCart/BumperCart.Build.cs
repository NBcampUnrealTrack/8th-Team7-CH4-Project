// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BumperCart : ModuleRules
{
	public BumperCart(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "OnlineSubsystemEOS",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "SlateCore",
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"BumperCart",
			"BumperCart/Cart",
			"BumperCart/Variant_Platforming",
			"BumperCart/Variant_Platforming/Animation",
			"BumperCart/Variant_Combat",
			"BumperCart/Variant_Combat/AI",
			"BumperCart/Variant_Combat/Animation",
			"BumperCart/Variant_Combat/Gameplay",
			"BumperCart/Variant_Combat/Interfaces",
			"BumperCart/Variant_Combat/UI",
			"BumperCart/Variant_SideScrolling",
			"BumperCart/Variant_SideScrolling/AI",
			"BumperCart/Variant_SideScrolling/Gameplay",
			"BumperCart/Variant_SideScrolling/Interfaces",
			"BumperCart/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
