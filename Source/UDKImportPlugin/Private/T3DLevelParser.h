#pragma once

#include "T3DParser.h"

class T3DMaterialParser;

class T3DLevelParser : public T3DParser
{
	friend class T3DMaterialParser;
public:
	T3DLevelParser(const FString &UdkPath, const FString &TmpPath);
	void ImportLevel(const FString &Level);
	void SetTexture(UObject * Object, UMaterialExpressionTextureBase * MaterialExpression);

private:
	/// Ressources requirements
	void ResolveRequirements();
	void ResolveMeshesRequirements();
	void ExportUDKAssets();
	TArray<UObject*> CreatedObjects;

	/// Actor creation
	UWorld * World;
	template<class T>
	T * SpawnActor();

	/// Actor Importation
	void ImportLevel();
	void ImportBrush();
	void ImportPolyList(UPolys * Polys);
	void ImportStaticMeshActor();
	void ImportPointLight();
	void ImportSpotLight();
	USoundCue * ImportSoundCue();

	/// Available ressource actions
	void SetStaticMesh(UObject * Object, UStaticMeshComponent * StaticMeshComponent);
	void SetPolygonTexture(UObject * Object, UPolys * Polys, int32 index);
	void SetSoundCueFirstNode(UObject * Object, USoundCue * SoundCue);
	void SetStaticMeshMaterial(UObject * Material, FString StaticMeshUrl, int32 MaterialIdx);
};
