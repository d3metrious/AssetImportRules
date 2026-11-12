// Copyright 2023-2026 Jimi Demetriou All Rights Reserved. Jimi Games | jimid.io

using UnrealBuildTool;

public class AssetImportRules : ModuleRules
{
	public AssetImportRules(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"DeveloperSettings"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"AssetRegistry",
			"InterchangeEngine"
		});
	}
}
