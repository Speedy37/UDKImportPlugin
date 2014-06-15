#include "UDKImportPluginPrivatePCH.h"
#include "T3DParser.h"

DEFINE_LOG_CATEGORY(UDKImportPluginLog);

float T3DParser::UnrRotToDeg = 0.00549316540360483;
float T3DParser::IntensityMultiplier = 5000;

T3DParser::T3DParser(const FString &UdkPath, const FString &TmpPath)
{
	this->UdkPath = UdkPath;
	this->TmpPath = TmpPath;
}

inline bool IsWhitespace(TCHAR c) 
{ 
	return c == LITERAL(TCHAR, ' ') || c == LITERAL(TCHAR, '\t') || c == LITERAL(TCHAR, '\r');
}

void T3DParser::ResetParser(const FString &Content)
{
	LineIndex = 0;
	ParserLevel = 0;
	Content.ParseIntoArray(&Lines, TEXT("\n"), true);
}

bool T3DParser::NextLine()
{
	if (LineIndex < Lines.Num())
	{
		int32 Start, End;
		const FString &String = Lines[LineIndex];
		Start = 0;
		End = String.Len();

		// Trimming
		while (Start < End && IsWhitespace(String[Start]))
		{
			++Start;
		}

		while (End > Start && IsWhitespace(String[End-1]))
		{
			--End;
		}

		Line = String.Mid(Start, End - Start);
		++LineIndex;
		return true;
	}
	return false;
}

bool T3DParser::IgnoreSubObjects()
{
	while (Line.StartsWith(TEXT("Begin Object "), ESearchCase::CaseSensitive))
	{
		JumpToEnd();
		if (!NextLine())
			return false;
	}

	return true;
}

bool T3DParser::IgnoreSubs()
{
	while (Line.StartsWith(TEXT("Begin "), ESearchCase::CaseSensitive))
	{
		JumpToEnd();
		if (!NextLine())
			return false;
	}

	return true;
}

void T3DParser::JumpToEnd()
{
	int32 Level = 1;
	while (NextLine())
	{
		if (Line.StartsWith(TEXT("Begin "), ESearchCase::CaseSensitive))
		{
			++Level;
		}
		else if (Line.StartsWith(TEXT("End "), ESearchCase::CaseSensitive))
		{
			--Level;
			if (Level == 0)
				break;
		}
	}
}

bool T3DParser::IsBeginObject(FString &Class)
{
	if (Line.StartsWith(TEXT("Begin Object "), ESearchCase::CaseSensitive))
	{
		GetOneValueAfter(TEXT(" Class="), Class);
		return true;
	}
	return false;
}

bool T3DParser::IsEndObject()
{
	return Line.Equals(TEXT("End Object"));
}

bool T3DParser::GetOneValueAfter(const FString &Key, FString &Value)
{
	int32 start = Line.Find(Key, ESearchCase::CaseSensitive, ESearchDir::FromStart, 0);
	if (start != -1)
	{
		start += Key.Len();

		const TCHAR * Buffer = *Line + start;
		while (*Buffer != TCHAR(' ') && *Buffer != TCHAR(',') && *Buffer != TCHAR(')'))
		{
			++Buffer;
		}
		Value = Line.Mid(start, Buffer - *Line - start);

		return true;
	}
	return false;
}

bool T3DParser::GetProperty(const FString &Key, FString &Value)
{
	if (Line.StartsWith(Key, ESearchCase::CaseSensitive))
	{
		Value = Line.Mid(Key.Len());
		return true;
	}
	return false;
}

void T3DParser::AddRequirement(const FString &UDKRequiredObjectName, UObjectDelegate Action)
{
	UObject ** pObject = FixedRequirements.Find(UDKRequiredObjectName);
	if (pObject != NULL)
	{
		Action.ExecuteIfBound(*pObject);
	}
	else
	{
		TArray<UObjectDelegate> * pActions = Requirements.Find(UDKRequiredObjectName);
		if (pActions != NULL)
		{
			pActions->Add(Action);
		}
		else
		{
			TArray<UObjectDelegate> Actions;
			Actions.Add(Action);
			Requirements.Add(UDKRequiredObjectName, Actions);
		}
	}
}

void T3DParser::FixRequirement(const FString &UDKRequiredObjectName, UObject * Object)
{
	if (Object == NULL)
		return;
	
	TArray<UObjectDelegate> * pActions = Requirements.Find(UDKRequiredObjectName);
	if (pActions != NULL)
	{
		for (auto IterActions = pActions->CreateConstIterator(); IterActions; ++IterActions)
		{
			IterActions->ExecuteIfBound(Object);
		}
		Requirements.Remove(UDKRequiredObjectName);
	}

	FixedRequirements.Add(UDKRequiredObjectName, Object);
}

bool T3DParser::FindRequirement(const FString &UDKRequiredObjectName, UObject * &Object)
{
	UObject ** pObject = FixedRequirements.Find(UDKRequiredObjectName);
	if (pObject != NULL)
	{
		Object = *pObject;
		return true;
	}

	return false;
}

void T3DParser::PrintMissingRequirements()
{
	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FString &Url = Iter.Key();
		UE_LOG(UDKImportPluginLog, Warning, TEXT("Missing requirements : %s"), *Url);
	}
}

bool T3DParser::ParseUDKRotation(const FString &InSourceString, FRotator &Rotator)
{
	int32 Pitch = 0;
	int32 Yaw = 0;
	int32 Roll = 0;

	const bool bSuccessful = FParse::Value(*InSourceString, TEXT("Pitch="), Pitch) && FParse::Value(*InSourceString, TEXT("Yaw="), Yaw) && FParse::Value(*InSourceString, TEXT("Roll="), Roll);

	Rotator.Pitch = Pitch * UnrRotToDeg;
	Rotator.Yaw = Yaw * UnrRotToDeg;
	Rotator.Roll = Roll * UnrRotToDeg;

	return bSuccessful;

}

bool T3DParser::ParseFVector(const TCHAR* Stream, FVector& Value)
{
	Value = FVector::ZeroVector;

	Value.X = FCString::Atof(Stream);
	Stream = FCString::Strchr(Stream, ',');
	if (!Stream)
	{
		return false;
	}

	Stream++;
	Value.Y = FCString::Atof(Stream);
	Stream = FCString::Strchr(Stream, ',');
	if (!Stream)
	{
		return false;
	}

	Stream++;
	Value.Z = FCString::Atof(Stream);

	return true;
}

bool T3DParser::IsProperty(FString &PropertyName, FString &Value)
{
	int32 Index;
	if (Line.FindChar('=', Index) && Index > 0)
	{
		PropertyName = Line.Mid(0, Index);
		Value = Line.Mid(Index + 1);
		return true;
	}

	return false;
}

bool T3DParser::IsActorLocation(AActor * Actor)
{
	FString Value;
	if (GetProperty(TEXT("Location="), Value))
	{
		FVector Location;
		ensure(Location.InitFromString(Value));
		Actor->SetActorLocation(Location);
		return true;
	}

	return false;
}

bool T3DParser::IsActorRotation(AActor * Actor)
{
	FString Value;
	if (GetProperty(TEXT("Rotation="), Value))
	{
		FRotator Rotator;
		ensure(ParseUDKRotation(Value, Rotator));
		Actor->SetActorRotation(Rotator);
		return true;
	}

	return false;
}

bool T3DParser::IsActorScale(AActor * Actor)
{
	FString Value;
	if (GetProperty(TEXT("DrawScale="), Value))
	{
		float DrawScale = FCString::Atof(*Value);
		Actor->SetActorScale3D(Actor->GetActorScale() * DrawScale);
		return true;
	}
	else if (GetProperty(TEXT("DrawScale3D="), Value))
	{
		FVector DrawScale3D;
		ensure(DrawScale3D.InitFromString(Value));
		Actor->SetActorScale3D(Actor->GetActorScale() * DrawScale3D);
		return true;
	}

	return false;
}

int32 T3DParser::RunUDK(const FString &CommandLine)
{
	FString Output;
	return RunUDK(CommandLine, Output);
}

int32 T3DParser::RunUDK(const FString &CommandLine, FString &Output)
{
	FString StdErr;
	int32 exitCode;

	if (FPlatformProcess::ExecProcess(*(UdkPath / TEXT("Binaries/Win32/UDK.com")), *CommandLine, &exitCode, &Output, &StdErr))
	{
		return exitCode;
	}

	return -1;
}

bool T3DParser::ConvertOBJToFBX(const FString &ObjFileName, const FString &FBXFilename)
{
	const FString CommandLine = FString::Printf(TEXT("\"%s\" \"%s\""), *ObjFileName, *FBXFilename);
	const FString Program = TEXT("C:\\Program Files (x86)\\Autodesk\\FBX\\FBX Converter\\2013.3\\bin\\FbxConverter.exe");
	FString StdOut, StdErr;
	int32 exitCode;

	if (FPlatformProcess::ExecProcess(*Program, *CommandLine, &exitCode, &StdOut, &StdErr))
	{
		return exitCode == 0;
	}

	return false;
}

bool T3DParser::ParseRessourceUrl(const FString &Url, FString &Type, FString &Package, FString &Name)
{
	int32 Index, PackageIndex, NameIndex;

	if (!Url.FindChar('\'', Index) || !Url.EndsWith(TEXT("'")))
		return false;

	Type = Url.Mid(0, Index);
	++Index;
	PackageIndex = Url.Find(".", ESearchCase::CaseSensitive, ESearchDir::FromStart, Index);

	if (PackageIndex == -1)
	{
		// Package Name is the current Package
		Package = this->Package;
		Name = Url.Mid(Index, Url.Len() - Index - 1);
	}
	else
	{
		Package = Url.Mid(Index, PackageIndex - Index);
		NameIndex = Url.Find(".", ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		Name = Url.Mid(NameIndex + 1, Url.Len() - NameIndex - 2);
	}

	return true;
}
