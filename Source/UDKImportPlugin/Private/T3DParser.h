#pragma once

struct T3DParser
{
public:
	T3DParser(const FString &T3DText);
	void ImportLevel();

private:
	static float UnrRotToDeg;
	static float IntensityMultiplier;

	/// Ressources requirements
	TMap<FString, FExecuteAction> Requirements;
	UObject * CurrentRequirement;
	void AddRequirement(const FString &UDKRequiredObjectName, FExecuteAction Action);

	/// Line parsing
	int32 LineIndex, ParserLevel;
	TArray<FString> Lines;
	FString Line;
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
