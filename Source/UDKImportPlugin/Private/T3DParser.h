#pragma once

struct T3DParser
{
public:
	T3DParser(const FString &UdkPath, const FString &TmpPath);
	void ImportLevel(const FString &Level);

private:
	static float UnrRotToDeg;
	static float IntensityMultiplier;
	int32 StatusNumerator, StatusDenominator;

	/// UDK
	FString UdkPath, TmpPath;
	int32 RunUDK(const FString &CommandLine);

	/// Ressources requirements
	TMap<FString, TArray<FExecuteAction>> Requirements;
	TMap<FString, UObject*> FixedRequirements;
	UObject * CurrentRequirement;
	bool ConvertOBJToFBX(const FString &ObjFileName, const FString &FBXFilename);
	void AddRequirement(const FString &UDKRequiredObjectName, FExecuteAction Action);
	void FixRequirement(const FString &UDKRequiredObjectName, UObject * Object);
	void ResolveRequirements();

	/// Line parsing
	int32 LineIndex, ParserLevel;
	TArray<FString> Lines;
	FString Line, Package;
	bool NextLine();
	bool IgnoreSubs();
	bool IgnoreSubObjects();
	void JumpToEnd();

	/// Line content parsing
	bool IsBeginObject(FString &Class, FString &Name);
	bool IsEndObject();
	bool IsActorLocation(AActor * Actor);
	bool IsActorRotation(AActor * Actor);
	bool IsActorScale(AActor * Actor);

	/// Value parsing
	bool GetOneValueAfter(const FString &Key, FString &Value);
	bool GetProperty(const FString &Key, FString &Value);
	bool ParseUDKRotation(const FString &InSourceString, FRotator &Rotator);
	bool ParseFVector(const TCHAR* Stream, FVector& Value);
	bool ParseRessourceUrl(const FString &Url, FString &Type, FString &Package, FString &Name);

	/// Actor creation
	UWorld * World;
	template<class T>
	T * SpawnActor();

	/// Actor Importation
	void ImportBrush();
	void ImportPolyList(UPolys * Polys);
	void ImportStaticMeshActor();
	void ImportPointLight();
	void ImportSpotLight();
	USoundCue * ImportSoundCue();

	/// Available ressource actions
	void SetStaticMesh(UStaticMeshComponent * StaticMeshComponent);
	void SetPolygonTexture(UPolys * Polys, int32 index);
	void SetSoundCueFirstNode(USoundCue * SoundCue);
};
