#pragma once

#define LOCTEXT_NAMESPACE "UDKImportPlugin"

DECLARE_LOG_CATEGORY_EXTERN(UDKImportPluginLog, Log, All);
DECLARE_DELEGATE_OneParam(UObjectDelegate, UObject*);

class T3DParser
{
public:
	struct FRequirement
	{
		FString Type, Package, Name, OriginalUrl, Url;
	};
protected:
	static float UnrRotToDeg;
	static float IntensityMultiplier;

	T3DParser(const FString &UdkPath, const FString &TmpPath);

	int32 StatusNumerator, StatusDenominator;

	/// UDK
	FString UdkPath, TmpPath;
	int32 RunUDK(const FString &CommandLine);
	int32 RunUDK(const FString &CommandLine, FString &output);

	/// Ressources requirements
	TMap<FRequirement, TArray<UObjectDelegate> > Requirements;
	TMap<FRequirement, UObject*> FixedRequirements;
	bool ConvertOBJToFBX(const FString &ObjFileName, const FString &FBXFilename);
	void AddRequirement(const FString &UDKRequiredObjectName, UObjectDelegate Action);
	void FixRequirement(const FString &UDKRequiredObjectName, UObject * Object);
	bool FindRequirement(const FString &UDKRequiredObjectName, UObject * &Object);
	void AddRequirement(const FRequirement &Requirement, UObjectDelegate Action);
	void FixRequirement(const FRequirement &Requirement, UObject * Object);
	bool FindRequirement(const FRequirement &Requirement, UObject * &Object);
	void PrintMissingRequirements();

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
	bool IsActorProperty(AActor * Actor);

	/// Value parsing
	bool GetOneValueAfter(const FString &Key, FString &Value, int32 maxindex = MAX_int32);
	bool GetProperty(const FString &Key, FString &Value);
	bool ParseUDKRotation(const FString &InSourceString, FRotator &Rotator);
	bool ParseFVector(const TCHAR* Stream, FVector& Value);
	void ParseRessourceUrl(const FString &Url, FString &Package, FString &Name);
	bool ParseRessourceUrl(const FString &Url, FString &Type, FString &Package, FString &Name);
	bool ParseRessourceUrl(const FString &Url, FRequirement &Requirement);
};

FORCEINLINE uint32 GetTypeHash(const T3DParser::FRequirement& R)
{
	return FCrc::Strihash_DEPRECATED(*(R.Url));
}

FORCEINLINE bool operator==(const T3DParser::FRequirement& A, const T3DParser::FRequirement& B)
{
	return A.Url == B.Url;
}

FORCEINLINE bool T3DParser::ParseRessourceUrl(const FString &Url, FRequirement &Requirement)
{
	Requirement.OriginalUrl = Url;
	if (ParseRessourceUrl(Url, Requirement.Type, Requirement.Package, Requirement.Name))
	{
		Requirement.Url = FString::Printf(TEXT("%s'%s.%s'"), *Requirement.Type, *Requirement.Package, *Requirement.Name);
		return true;
	}
	return false;
}

FORCEINLINE bool T3DParser::GetProperty(const FString &Key, FString &Value)
{
	return GetOneValueAfter(Key, Value, 0);
}
