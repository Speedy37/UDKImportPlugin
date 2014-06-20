UDKImportPlugin for Unreal Engine 4
======================

UDKImportPlugin is a module for the Unreal Engine 4 editor providing a  tool for migrating maps for the Unreal Developement Kit (UDK). This module has been made for the [Stargate Network](http://www.stargate-network.net/) and Stargate No Limits teams to help them migrates their contents to the Unreal Engine 4.

If an EpicGame dev show this : 
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
 - Some materials expression are supported (MaterialExpressionTextureFlipBook for example)
 - Statimeshes can be automatically exported, but because the batchexport commandlet of the UDK does'nt produce valid FBX files, the tool export them in OBJ. Then use the Autodesk FBX Converter to convert the OBJ files to FBX for the importation. As a workaround, you export one by one asset from the UDK Content browser to FBX file. The tool will check for FBX file before OBJ, so you can still auto import non degraded mesh. 

How to use
----------

If you want the plugin beeing able to do the automatic OBJ exportation, you must install Autodesk FBX Converter 2013 (32 bits).

1. To be able to find which materials are used by a specific StaticMesh, a UDK Commandlet has to be added. To do so, you have to copy the `UDKPluginExport` folder to the `UDKPath/Development/Src` folder. Then in the `UDKPath/UDKGame/Config/DefautEngineUDK.ini` file set the `ModEditPackages` value to `UDKPluginExport` in the `[UnrealEd.EditorEngine]` category. Now run the UDK make command to build the UDK commandlet.
2. In your Unreal Engine 4 project, create the `Plugins` folder and clone this project.
You should have something like this : `MyProject/Plugins/UDKImportPlugin`
3. Then simply right click on your .uproject file and "Generate Visual Studio Project" to force generation of valid Visual Studio project with the plugin inside.
4. Now start your project. While starting the editor should build the plugin

Tips
----

You should export the staticmesh to FBX using the UDK Content Browser. An simple keyboard macro : {TAB}{DOWN}{ENTER} can help you do this job.
