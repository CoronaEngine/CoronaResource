// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;

namespace UnrealBuildTool.Rules
{
	public class ParametricSurfaceExtension : ModuleRules
	{
		public ParametricSurfaceExtension(ReadOnlyTargetRules Target) : base(Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
				}
			);

			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"CoreUObject",
					"CADLibrary",
					"DatasmithContent",
					"Engine",
					"MeshDescription",
					"ParametricSurface",
					"PhysicsCore",
					"StaticMeshDescription",
				}
			);

			if (Target.bBuildEditor)
			{
				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"ContentBrowser",
						"DataprepCore",
						"DatasmithImporter",
						"EditorFramework",
						"Slate",
						"SlateCore",
						"StaticMeshEditor",
						"ToolMenus",
						"UnrealEd",
					}
				);
			}
		}
	}
}
