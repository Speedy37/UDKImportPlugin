#pragma once

#define LOCTEXT_NAMESPACE "UDKImportPlugin"

DECLARE_DELEGATE_OneParam(UObjectDelegate, UObject*);

class T3DParser
{
protected:
	static float UnrRotToDeg;
	static float IntensityMultiplier;

	T3DParser(const FString &UdkPath, const FString &TmpPath);
	T3DParser(T3DParser * ParentParser);

	int32 StatusNumerator, StatusDenominator;

	/// UDK
	FString UdkPath, TmpPath;
	int32 RunUDK(const FString &CommandLine);
	int32 RunUDK(const FString &CommandLine, FString &output);

	/// Ressources requirements
	TMap<FString, TArray<UObjectDelegate>> Requirements;
	TMap<FString, UObject*> FixedRequirements;
	bool ConvertOBJToFBX(const FString &ObjFileName, const FString &FBXFilename);
	void AddRequirement(const FString &UDKRequiredObjectName, UObjectDelegate Action);
	void FixRequirement(const FString &UDKRequiredObjectName, UObject * Object);
	bool FindRequirement(const FString &UDKRequiredObjectName, UObject * &Object);

	/// Line parsing
	int32 LineIndex, ParserLevel;
	TArray<FString> Lines;
	FString Line, Package;
	void ResetParser(const FString &Content);
	bool NextLine();
	bool IgnoreSubs();
	bool IgnoreSubObjects();
	void JumpToEnd();

	/// Line content parsing
	bool IsBeginObject(FString &Class);
	bool IsEndObject();
	bool IsProperty(FString &PropertyName, FString &Value);
	bool IsActorLocation(AActor * Actor);
	bool IsActorRotation(AActor * Actor);
	bool IsActorScale(AActor * Actor);

	/// Value parsing
	bool GetOneValueAfter(const FString &Key, FString &Value);
	bool GetProperty(const FString &Key, FString &Value);
	bool ParseUDKRotation(const FString &InSourceString, FRotator &Rotator);
	bool ParseFVector(const TCHAR* Stream, FVector& Value);
	bool ParseRessourceUrl(const FString &Url, FString &Type, FString &Package, FString &Name);
};
