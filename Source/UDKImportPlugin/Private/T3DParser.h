#pragma once

struct T3DParser
{
public:
	T3DParser(const FString &T3DText);
	void ImportLevel();

private:
	static float UnrRotToDeg;
	static float IntensityMultiplier;

	TArray<FString> Lines;
	FString Line;

	int32 LineIndex, ParserLevel;
	TMap<FString, FExecuteAction> Requirements;
	UObject * CurrentRequirement;
	UWorld * World;

	bool NextLine();
	bool IgnoreSubs();
	bool IgnoreSubObjects();
	void JumpToEnd();
	bool IsBeginObject(FString &Class, FString &Name);
	bool IsEndObject();
	bool GetOneValueAfter(const FString &Key, FString &Value);
	bool GetProperty(const FString &Key, FString &Value);
	void AddRequirement(const FString &UDKRequiredObjectName, FExecuteAction Action);
	bool ParseUDKRotation(const FString &InSourceString, FRotator &Rotator);
	bool ParseFVector(const TCHAR* Stream, FVector& Value);
	bool IsActorLocation(AActor * Actor);
	bool IsActorRotation(AActor * Actor);
	bool IsActorScale(AActor * Actor);

	template<class T>
	T * SpawnActor();

	void ImportBrush();
	void ImportPolyList(UPolys * Polys);
	void ImportStaticMeshActor();
	void ImportPointLight();
	void ImportSpotLight();
	USoundCue * ImportSoundCue();
	void SetStaticMesh(UStaticMeshComponent * StaticMeshComponent);
	void SetPolygonTexture(UPolys * Polys, int32 index);
	void SetSoundCueFirstNode(USoundCue * SoundCue);
};
