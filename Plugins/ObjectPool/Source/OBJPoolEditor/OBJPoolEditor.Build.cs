using UnrealBuildTool;

public class OBJPoolEditor : ModuleRules {
    public OBJPoolEditor(ReadOnlyTargetRules Target) : base(Target) {
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bEnforceIWYU = true;
		//
        PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "Engine",
                "CoreUObject",
                "OBJPool"
            }
        );
        //
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Slate",
                "Projects",
                "UnrealEd",
                "SlateCore",
                "InputCore",
                "AssetTools",
                "EditorStyle",
                "LevelEditor",
                "PropertyEditor",
				"BlueprintGraph",
				"KismetCompiler"
            }
        );
    }
}