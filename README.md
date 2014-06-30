UDK Import Plugin for Unreal Engine 4
======================

UDKImportPlugin is a module for the Unreal Engine 4 editor providing a  tool for migrating maps for the Unreal Developement Kit (UDK). This module has been made for the [Stargate Network](http://www.stargate-network.net/) and Stargate No Limits teams to help them migrates their contents to the Unreal Engine 4.

If an EpicGame dev reads this : 
> Why the batchexport PackageName StaticMesh FBX command produce empty files ?

It supports migrating :
 - Brushs, 
 - StaticMeshes, 
 - Materials, 
 - Textures, 
 - PointLights, 
 - SpotLights

Limitations
-----------

Currently this tool has some limitations :
 - The brush csg order is lost in the process, so you may have to rearange the brush order
 - Statimeshes can be automatically exported, but because the batchexport commandlet of the UDK does not produce valid FBX files, the tool export them in OBJ. Then use the Autodesk FBX Converter to convert the OBJ files to FBX for the importation. As a workaround, you export one by one asset from the UDK Content browser to FBX file. The tool will check for FBX file before OBJ, so you can still auto import non degraded mesh. 

How to install
----------

If you want the plugin beeing able to do the automatic OBJ exportation, you must install Autodesk FBX Converter 2013 (32 bits).

1. To be able to find which materials are used by a specific StaticMesh, a UDK Commandlet has to be added. To do so, you have to copy the `UDKPluginExport` folder to the `UDKPath/Development/Src` folder. Then in the `UDKPath/UDKGame/Config/DefautEngineUDK.ini` file set the `ModEditPackages` value to `UDKPluginExport` in the `[UnrealEd.EditorEngine]` category. Now run the UDK make command to build the UDK commandlet.
2. In your Unreal Engine 4 project, create the `Plugins` folder and clone this project.
You should have something like this : `MyProject/Plugins/UDKImportPlugin`
3. Then simply right click on your .uproject file and "Generate Visual Studio Project" to force generation of valid Visual Studio project with the plugin inside.
4. Now start your project. While starting the editor should build the plugin
5. Activate the plugin via the Plugin Manager and restart the editor
6. To access to the import tool go to : `Help > UDKImport`

How to use
----------
1. First of all, configure the tool
2. Do a first run and checks the log for errors
3. StaticMeshes are automatically exported as OBJ. To have a good result, you must export them manually as FBX. So go to your temporary directory, the folder `ExportedMeshes` should contains a folder for package required by the exportation. Start your UDK project, and do a manual exportation of thoses assets. To do so, you can select all the asset that are in a package, `RightClick > Export to file` select FBX as export format and `Enter`. Because this process is slow I recommand you to build a very simple keyboard macro : `{TAB}{f}{ENTER}`. It's the only step that has to be done manually.
4. Once StaticMeshes are all exported in FBX, simply run the tool again.

Tips
----

The tool doesn't create a new map, so don't forget to create a new empty map before running the tool.

The temporary directory is reused, you don't need and don't want to make it empty again unless you have made changes in your UDK packages or after a plugin updates. Don't forget to save your exported FBX staticmeshes to limit the time lost updating the exported FBXs.

You should export the staticmesh to FBX using the UDK Content Browser. An simple keyboard macro : {TAB}{f}{ENTER} can help you do this job.
