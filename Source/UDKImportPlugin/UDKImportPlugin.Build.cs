using UnrealBuildTool;
using System.IO;
 
namespace UnrealBuildTool.Rules
{
	public class UDKImportPlugin : ModuleRules
	{
        public UDKImportPlugin(TargetInfo Target)
		{
			PublicDependencyModuleNames.AddRange(
				new string[] {
					"Core",
					"CoreUObject",		// @todo Mac: for some reason CoreUObject and Engine are needed to link in debug on Mac
                    "InputCore",
					"Engine",
					"Slate"
				}
			);
			
			PrivateDependencyModuleNames.AddRange(
				new string[] {
					"EditorStyle",
					"Projects",
					"UnrealEd",
					"LevelEditor",
					"AssetTools",
				}
			);
		}
	}
}