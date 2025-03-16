// Copyright 2024, Aquanox.

using UnrealBuildTool;

public class BlueprintComponentReferenceTests : ModuleRules
{
	// This is to emulate engine installation and verify includes during development
	// Gives effect similar to BuildPlugin with -StrictIncludes
	public bool bStrictIncludesCheck = false;

	public BlueprintComponentReferenceTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		if (bStrictIncludesCheck)
		{
			bUseUnity = false;
			PCHUsage = PCHUsageMode.NoPCHs;
			// Enable additional checks used for Engine modules
			bTreatAsEngineModule = true;
		}

		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] {
				"Core",
				"CoreUObject",
				"Engine",
				"GameplayTags",
				"BlueprintComponentReference",
				"BlueprintComponentReferenceEditor"
		});

		if (Target.Version.MajorVersion >= 5)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"AutomationTest"
			});
		}
	}
}
