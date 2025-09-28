// Copyright Epic Games, Inc. All Rights Reserved.
using System.IO;

namespace UnrealBuildTool.Rules
{
	public class CADKernel : ModuleRules
	{
		public CADKernel(ReadOnlyTargetRules Target)
			: base(Target)
		{
			//OptimizeCode = CodeOptimization.Never;
			CppCompileWarningSettings.DeterministicWarningLevel = WarningLevel.Off; // __DATE__ in Private/CADKernel/Core/System.cpp
			
			PublicDefinitions.Add("CADKERNEL_THINZONE=0");

			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"GeometryCore",
				}
			);

			PublicDefinitions.Add(Target.Type == TargetType.Program ? "CADKERNEL_DO_ENSURE=0" : "CADKERNEL_DO_ENSURE=1");
		}
	}
}