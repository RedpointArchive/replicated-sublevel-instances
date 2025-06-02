// Copyright 2025 June Rhodes. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Reflection;

public class ReplicatedSublevelInstances : ModuleRules
{
    public ReplicatedSublevelInstances(ReadOnlyTargetRules Target) : base(Target)
    {
		// Ensure we're always using the latest build and include order versions, so that our
		// CI/CD server can pick up issues in new engine releases.
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
            }
        );

#if UE_5_2_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_5_2=1");
#elif UE_5_1_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_5_1=1");
#elif UE_5_0_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_5_0=1");
#elif UE_4_27_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_27=1");
#elif UE_4_26_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_26=1");
#elif UE_4_25_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_25=1");
#elif UE_4_24_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_24=1");
#elif UE_4_23_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_23=1");
#elif UE_4_22_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_22=1");
#elif UE_4_21_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_21=1");
#elif UE_4_20_OR_LATER
		PublicDefinitions.Add("IS_UNREAL_4_20=1");
#endif
#if UE_4_21_OR_LATER
		PublicDefinitions.Add("NEWER_LEVEL_STREAMING_CLASS=1");
#endif
    }
}