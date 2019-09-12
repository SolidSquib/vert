// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Vert : ModuleRules
{
	public Vert(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "CableComponent", "OBJPool", "UMG", "Slate", "SlateCore", "AkAudio", "MoviePlayer" });
        PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem"/*, "Slate", "SlateCore" */ });

        PublicIncludePathModuleNames.AddRange(new string[] { "OBJPool" });

        if(UEBuildConfiguration.bBuildEditor == true)
        {
            PublicDependencyModuleNames.Add("OBJPoolEditor");
            PublicIncludePathModuleNames.Add("OBJPoolEditor");
        }
    }
}
