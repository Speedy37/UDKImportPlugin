#include "UDKImportPluginPrivatePCH.h"
#include "T3DParser.h"
#include "Editor/UnrealEd/Public/BSPOps.h"

#define LOCTEXT_NAMESPACE "UDKImportPlugin"

float T3DParser::UnrRotToDeg = 0.00549316540360483;
float T3DParser::IntensityMultiplier = 5000;

T3DParser::T3DParser(const FString &UdkPath, const FString &TmpPath)
{
	this->CurrentRequirement = NULL;
	this->World = NULL;
	this->UdkPath = UdkPath;
	this->TmpPath = TmpPath;
}

inline bool IsWhitespace(TCHAR c) 
{ 
	return c == LITERAL(TCHAR, ' ') || c == LITERAL(TCHAR, '\t') || c == LITERAL(TCHAR, '\r');
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

bool T3DParser::IsBeginObject(FString &Class, FString &Name)
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
		int32 end = Line.Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromStart, start);
		if (end == -1)
			end = Line.Len();
		Value = Line.Mid(start, end - start);

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

void T3DParser::AddRequirement(const FString &UDKRequiredObjectName, FExecuteAction Action)
{
	UObject ** pObject = FixedRequirements.Find(UDKRequiredObjectName);
	if (pObject != NULL)
	{
		CurrentRequirement = *pObject;
		Action.ExecuteIfBound();
	}
	else
	{
		TArray<FExecuteAction> * pActions = Requirements.Find(UDKRequiredObjectName);
		if (pActions != NULL)
		{
			pActions->Add(Action);
		}
		else
		{
			TArray<FExecuteAction> Actions;
			Actions.Add(Action);
			Requirements.Add(UDKRequiredObjectName, Actions);
		}
	}
}


void T3DParser::FixRequirement(const FString &UDKRequiredObjectName, UObject * Object)
{
	if (Object == NULL)
		return;
	
	TArray<FExecuteAction> * pActions = Requirements.Find(UDKRequiredObjectName);
	if (pActions != NULL)
	{
		CurrentRequirement = Object;
		for (auto IterActions = pActions->CreateConstIterator(); IterActions; ++IterActions)
		{
			IterActions->ExecuteIfBound();
		}
		pActions->Empty();
	}

	FixedRequirements.Add(UDKRequiredObjectName, Object);
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

template<class T>
T * T3DParser::SpawnActor()
{
	if (World == NULL)
	{
		FLevelEditorModule & LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		World = LevelEditorModule.GetFirstLevelEditor().Get()->GetWorld();
	}
	ensure(World != NULL);

	return World->SpawnActor<T>();
}

void T3DParser::ImportLevel(const FString &Level)
{
	FString Class, Name, UdkLevelT3D;
	const FString CommandLine = FString::Printf(TEXT("batchexport %s Level T3D %s"), *Level, *TmpPath);

	StatusNumerator = 0;
	StatusDenominator = 6;
	GWarn->BeginSlowTask(LOCTEXT("StatusBegin", "Importing requested level"), true, false);
	
	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportUDKLevelT3D", "Exporting UDK Level informations"));
	if (0 && RunUDK(CommandLine) != 0)
		return;

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("LoadUDKLevelT3D", "Loading UDK Level informations"));
	if (!FFileHelper::LoadFileToString(UdkLevelT3D, *(TmpPath / TEXT("PersistentLevel.T3D"))))
		return;

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ParsingUDKLevelT3D", "Parsing UDK Level informations"));
	LineIndex = 0;
	ParserLevel = 0;
	Package = Level;
	UdkLevelT3D.ParseIntoArray(&Lines, TEXT("\n"), true);
	UdkLevelT3D.Empty();

	ensure(NextLine());
	ensure(Line.Equals(TEXT("Begin Object Class=Level Name=PersistentLevel")));

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class, Name))
		{
			UObject * Object = 0;
			if (Class.Equals(TEXT("StaticMeshActor")))
				ImportStaticMeshActor();
			else if (Class.Equals(TEXT("Brush")))
				ImportBrush();
			else if (Class.Equals(TEXT("PointLight")))
				ImportPointLight();
			else if (Class.Equals(TEXT("SpotLight")))
				ImportSpotLight();
			else
				JumpToEnd();
		}
	}

	ResolveRequirements();
	GWarn->EndSlowTask();
}

int32 T3DParser::RunUDK(const FString &CommandLine)
{
	int32 exitCode = -1;
	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(*(UdkPath / TEXT("Binaries/Win32/UDK.com")), *CommandLine, false, false, false, NULL, 0, NULL, NULL);

	if (ProcessHandle.IsValid())
	{
		FPlatformProcess::WaitForProc(ProcessHandle);
		FPlatformProcess::GetProcReturnCode(ProcessHandle, &exitCode);
	}

	return exitCode;
}

bool T3DParser::ConvertOBJToFBX(const FString &ObjFileName, const FString &FBXFilename)
{
	int32 exitCode = -1;
	FString CommandLine = FString::Printf(TEXT("\"%s\" \"%s\""), *ObjFileName, *FBXFilename);

	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(TEXT("C:\\Program Files (x86)\\Autodesk\\FBX\\FBX Converter\\2013.3\\bin\\FbxConverter.exe"), *CommandLine, false, true, true, NULL, 0, NULL, NULL);
	if (ProcessHandle.IsValid())
	{
		FPlatformProcess::WaitForProc(ProcessHandle);
		if (FPlatformProcess::GetProcReturnCode(ProcessHandle, &exitCode) && exitCode == 0)
		{
			return true;
		}
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

void T3DParser::ResolveRequirements()
{
	bool Continue = true;
	TSet<FString> ExportedOBJStaticMeshPackages;
	TArray<FString> StaticMeshFiles;
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	IFileManager & FileManager = IFileManager::Get();
	FString Url, Type, PackageName, Name, ExportFolder, ImportFolder, FileName;

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportUDKAssets", "Exporting UDK Assets"));
	for (auto Iter = Requirements.CreateConstIterator(); Iter && Continue; ++Iter)
	{
		Url = Iter.Key();
		ParseRessourceUrl(Url, Type, PackageName, Name);

		if (Type == TEXT("StaticMesh"))
		{
			ExportFolder = TmpPath / TEXT("ExportedMeshes") / PackageName;
			ImportFolder = TmpPath / TEXT("Meshes") / PackageName;
			FileName = Name + TEXT(".OBJ");
			
			if (!ExportedOBJStaticMeshPackages.Contains(PackageName))
			{
				if (FileManager.DirectoryExists(*ExportFolder)
					|| RunUDK(FString::Printf(TEXT("batchexport %s StaticMesh OBJ %s"), *PackageName, *ExportFolder)) == 0)
				{
					ExportedOBJStaticMeshPackages.Add(PackageName);
				}
			}

			if (FileManager.FileSize(*(ExportFolder / FileName)) > 0)
			{
				if (FileManager.MakeDirectory(*ImportFolder, true))
				{
					if (FileManager.FileSize(*(ImportFolder / Name + TEXT(".FBX"))) == INDEX_NONE)
					{
						ConvertOBJToFBX(ExportFolder / FileName, ImportFolder / Name + TEXT(".FBX"));
					}
				}
			}
		}
	}

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ImportStaticMesh", "Importing Meshes"));
	StaticMeshFiles.Add(TmpPath / TEXT("Meshes"));
	AssetToolsModule.Get().ImportAssets(StaticMeshFiles, TEXT("/Game/UDK"));
	
	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ResolvingLinks", "Updating actors assets"));
	for (auto Iter = Requirements.CreateConstIterator(); Iter && Continue; ++Iter)
	{
		Url = Iter.Key();
		ParseRessourceUrl(Url, Type, PackageName, Name);

		if (Type == TEXT("StaticMesh"))
		{
			FString ObjectPath = FString::Printf(TEXT("/Game/UDK/Meshes/%s/%s.%s"), *PackageName, *Name, *Name);
			FixRequirement(Url, FindObject<UStaticMesh>(NULL, *ObjectPath));
		}
	}
}

void T3DParser::ImportBrush()
{
	FString Value, Class, Name;
	ABrush * Brush = SpawnActor<ABrush>();
	Brush->BrushType = Brush_Add;
	UModel* Model = new(Brush, NAME_None, RF_Transactional)UModel(FPostConstructInitializeProperties(), Brush, 1);

	while (NextLine() && !IsEndObject())
	{
		if (Line.StartsWith(TEXT("Begin Brush ")))
		{
			while (NextLine() && !Line.StartsWith(TEXT("End Brush")))
			{
				if (Line.StartsWith(TEXT("Begin PolyList")))
				{
					ImportPolyList(Model->Polys);
				}
			}
		}
		else if (GetProperty(TEXT("CsgOper="), Value))
		{
			if (Value.Equals(TEXT("CSG_Subtract")))
			{
				Brush->BrushType = Brush_Subtract;
			}
		}
		else if (IsActorLocation(Brush))
		{
			continue;
		}
		else if (Line.StartsWith(TEXT("Begin "), ESearchCase::CaseSensitive))
		{
			JumpToEnd();
		}
	}
	
	Model->Modify();
	if (!Model->Linked)
	{
		Model->Linked = 1;
		for (int32 i = 0; i<Model->Polys->Element.Num(); i++)
		{
			Model->Polys->Element[i].iLink = i;
		}
		int32 n = 0;
		for (int32 i = 0; i<Model->Polys->Element.Num(); i++)
		{
			FPoly* EdPoly = &Model->Polys->Element[i];
			if (EdPoly->iLink == i)
			{
				for (int32 j = i + 1; j<Model->Polys->Element.Num(); j++)
				{
					FPoly* OtherPoly = &Model->Polys->Element[j];
					if
						(OtherPoly->iLink == j
						&&	OtherPoly->Material == EdPoly->Material
						&&	OtherPoly->TextureU == EdPoly->TextureU
						&&	OtherPoly->TextureV == EdPoly->TextureV
						&&	OtherPoly->PolyFlags == EdPoly->PolyFlags
						&& (OtherPoly->Normal | EdPoly->Normal)>0.9999)
					{
						float Dist = FVector::PointPlaneDist(OtherPoly->Vertices[0], EdPoly->Vertices[0], EdPoly->Normal);
						if (Dist>-0.001 && Dist<0.001)
						{
							OtherPoly->iLink = i;
							n++;
						}
					}
				}
			}
		}
	}

	// Build bounds.
	Model->BuildBound();

	Brush->BrushComponent->Brush = Brush->Brush;

	// Let the actor deal with having been imported, if desired.
	Brush->PostEditImport();
	// Notify actor its properties have changed.
	Brush->PostEditChange();
	Brush->SetFlags(RF_Standalone | RF_Public);

}

void T3DParser::ImportPolyList(UPolys * Polys)
{
	FString Texture;
	while (NextLine() && !Line.StartsWith(TEXT("End PolyList")))
	{
		if (Line.StartsWith(TEXT("Begin Polygon ")))
		{
			bool GotBase = false;
			FPoly Poly;
			if (GetOneValueAfter(TEXT(" Texture="), Texture))
			{
				AddRequirement(FString::Printf(TEXT("Material'%s'"), *Texture), FExecuteAction::CreateRaw(this, &T3DParser::SetPolygonTexture, Polys, Polys->Element.Num()));
			}
			FParse::Value(*Line, TEXT("LINK="), Poly.iLink);
			Poly.PolyFlags &= ~PF_NoImport;

			while (NextLine() && !Line.StartsWith(TEXT("End Polygon")))
			{
				const TCHAR* Str = *Line;
				if (FParse::Command(&Str, TEXT("ORIGIN")))
				{
					GotBase = true;
					ParseFVector(Str, Poly.Base);
				}
				else if (FParse::Command(&Str, TEXT("VERTEX")))
				{
					FVector TempVertex;
					ParseFVector(Str, TempVertex);
					new(Poly.Vertices) FVector(TempVertex);
				}
				else if (FParse::Command(&Str, TEXT("TEXTUREU")))
				{
					ParseFVector(Str, Poly.TextureU);
				}
				else if (FParse::Command(&Str, TEXT("TEXTUREV")))
				{
					ParseFVector(Str, Poly.TextureV);
				}
				else if (FParse::Command(&Str, TEXT("NORMAL")))
				{
					ParseFVector(Str, Poly.Normal);
				}
			}
			if (!GotBase)
				Poly.Base = Poly.Vertices[0];
			if (Poly.Finalize(NULL, 1) == 0)
				new(Polys->Element)FPoly(Poly);
		}
	}
}

void T3DParser::ImportPointLight()
{
	FString Value, Class, Name;
	APointLight* PointLight = SpawnActor<APointLight>();

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class, Name))
		{
			if (Class.Equals(TEXT("SpotLightComponent")))
			{
				while (NextLine() && IgnoreSubs() && !IsEndObject())
				{
					if (GetProperty(TEXT("Radius="), Value))
					{
						PointLight->PointLightComponent->AttenuationRadius = FCString::Atof(*Value);
					}
					else if (GetProperty(TEXT("Brightness="), Value))
					{
						PointLight->PointLightComponent->Intensity = FCString::Atof(*Value) * IntensityMultiplier;
					}
					else if (GetProperty(TEXT("LightColor="), Value))
					{
						FColor Color;
						Color.InitFromString(Value);
						PointLight->PointLightComponent->LightColor = Color;
					}
				}
			}
			else
			{
				JumpToEnd();
			}
		}
		else if (IsActorLocation(PointLight) || IsActorRotation(PointLight))
		{
			continue;
		}
	}
}

void T3DParser::ImportSpotLight()
{
	FVector DrawScale3D(1.0,1.0,1.0);
	FRotator Rotator(0.0, 0.0, 0.0);
	FString Value, Class, Name;
	ASpotLight* SpotLight = SpawnActor<ASpotLight>();

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class, Name))
		{
			if (Class.Equals(TEXT("SpotLightComponent")))
			{
				while (NextLine() && IgnoreSubs() && !IsEndObject())
				{
					if (GetProperty(TEXT("Radius="), Value))
					{
						SpotLight->SpotLightComponent->AttenuationRadius = FCString::Atof(*Value);
					}
					else if (GetProperty(TEXT("InnerConeAngle="), Value))
					{
						SpotLight->SpotLightComponent->InnerConeAngle = FCString::Atof(*Value);
					}
					else if (GetProperty(TEXT("OuterConeAngle="), Value))
					{
						SpotLight->SpotLightComponent->OuterConeAngle = FCString::Atof(*Value);
					}
					else if (GetProperty(TEXT("Brightness="), Value))
					{
						SpotLight->SpotLightComponent->Intensity = FCString::Atof(*Value) * IntensityMultiplier;
					}
					else if (GetProperty(TEXT("LightColor="), Value))
					{
						FColor Color;
						Color.InitFromString(Value);
						SpotLight->SpotLightComponent->LightColor = Color;
					}
				}
			}
			else
			{
				JumpToEnd();
			}
		}
		else if (IsActorLocation(SpotLight))
		{
			continue;
		}
		else if (GetProperty(TEXT("Rotation="), Value))
		{
			ensure(ParseUDKRotation(Value, Rotator));
		}
		else if (GetProperty(TEXT("DrawScale3D="), Value))
		{
			ensure(DrawScale3D.InitFromString(Value));
		}
	}

	// Because there is people that does this in UDK...
	SpotLight->SetActorRotation((DrawScale3D.X * Rotator.Vector()).Rotation());
}

void T3DParser::ImportStaticMeshActor()
{
	FString Value, Class, Name;
	AStaticMeshActor * StaticMeshActor = SpawnActor<AStaticMeshActor>();
	
	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class, Name))
		{
			if (Class.Equals(TEXT("StaticMeshComponent")))
			{
				while (NextLine() && !IsEndObject())
				{
					if (GetProperty(TEXT("StaticMesh="), Value))
					{
						AddRequirement(Value, FExecuteAction::CreateRaw(this, &T3DParser::SetStaticMesh, StaticMeshActor->StaticMeshComponent.Get()));
					}
				}
			}
			else
			{
				JumpToEnd();
			}
		}
		else if (IsActorLocation(StaticMeshActor) || IsActorRotation(StaticMeshActor) || IsActorScale(StaticMeshActor))
		{
			continue;
		}
	}
}

USoundCue * T3DParser::ImportSoundCue()
{
	USoundCue * SoundCue = 0;
	FString Value;

	while (NextLine())
	{
		if (GetProperty(TEXT("SoundClass="), Value))
		{
			// TODO
		}
		else if (GetProperty(TEXT("FirstNode="), Value))
		{
			AddRequirement(Value, FExecuteAction::CreateRaw(this, &T3DParser::SetSoundCueFirstNode, SoundCue));
		}
	}

	return SoundCue;
}

void T3DParser::SetPolygonTexture(UPolys * Polys, int32 index)
{
	Polys->Element[index].Material = Cast<UMaterialInterface>(CurrentRequirement);
}

void T3DParser::SetStaticMesh(UStaticMeshComponent * StaticMeshComponent)
{
	UProperty* ChangedProperty = FindField<UProperty>(UStaticMeshComponent::StaticClass(), "StaticMesh");
	UStaticMesh * StaticMesh = Cast<UStaticMesh>(CurrentRequirement);

	StaticMeshComponent->PreEditChange(ChangedProperty);
	StaticMeshComponent->StaticMesh = StaticMesh;
	FPropertyChangedEvent PropertyChangedEvent(ChangedProperty);
	StaticMeshComponent->PostEditChangeProperty(PropertyChangedEvent);
}

void T3DParser::SetSoundCueFirstNode(USoundCue * SoundCue)
{
	SoundCue->FirstNode = Cast<USoundNode>(CurrentRequirement);
}