#include "UDKImportPluginPrivatePCH.h"
#include "Editor/UnrealEd/Public/BSPOps.h"
#include "Runtime/Engine/Public/ComponentReregisterContext.h"
#include "Runtime/Engine/Classes/Sound/SoundNode.h"
#include "T3DLevelParser.h"
#include "T3DMaterialParser.h"
#include "T3DMaterialInstanceConstantParser.h"

T3DLevelParser::T3DLevelParser(const FString &UdkPath, const FString &TmpPath) : T3DParser(UdkPath, TmpPath)
{
	this->World = NULL;
}

template<class T>
T * T3DLevelParser::SpawnActor()
{
	if (World == NULL)
	{
		FLevelEditorModule & LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		World = LevelEditorModule.GetFirstLevelEditor().Get()->GetWorld();
	}
	ensure(World != NULL);

	return World->SpawnActor<T>();
}

void T3DLevelParser::ImportLevel(const FString &Level)
{
	GWarn->BeginSlowTask(LOCTEXT("StatusBeginLevel", "Importing requested level"), true, false);
	StatusNumerator = 0;
	StatusDenominator = 12;
	
	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportUDKLevelT3D", "Exporting UDK Level informations"));
	{
		const FString CommandLine = FString::Printf(TEXT("batchexport %s Level T3D %s"), *Level, *TmpPath);
		if (RunUDK(CommandLine) != 0)
		{
			GWarn->EndSlowTask();
			return;
		}
	}

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("LoadUDKLevelT3D", "Loading UDK Level informations"));
	{
		FString UdkLevelT3D;
		if (!FFileHelper::LoadFileToString(UdkLevelT3D, *(TmpPath / TEXT("PersistentLevel.T3D"))))
		{
			GWarn->EndSlowTask();
			return;
		}

		ResetParser(UdkLevelT3D);
		Package = Level;
	}

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ParsingUDKLevelT3D", "Parsing UDK Level informations"));
	ImportLevel();

	ResolveRequirements();
	GWarn->EndSlowTask();
}

void T3DLevelParser::ImportStaticMesh(const FString &StaticMesh)
{
	ImportRessource(StaticMesh, EExportType::StaticMesh);
}

void T3DLevelParser::ImportMaterial(const FString &Material)
{
	ImportRessource(Material, EExportType::Material);
}

void T3DLevelParser::ImportMaterialInstanceConstant(const FString &MaterialInstanceConstant)
{
	ImportRessource(MaterialInstanceConstant, EExportType::MaterialInstanceConstant);
}

void T3DLevelParser::ImportRessource(const FString &Ressource, EExportType::Type Type)
{
	StatusNumerator = 0;
	StatusDenominator = 9;

	FString Name;
	ParseRessourceUrl(Ressource, Package, Name);
	if (Name.Len() > 0)
	{
		GWarn->BeginSlowTask(LOCTEXT("StatusBeginMaterialRessouce", "Importing requested ressource"), true, false);
		AddRequirement(FString::Printf(TEXT("%s'%s.%s'"), *RessourceTypeFor(Type), *Package, *Name), UObjectDelegate());
	}
	else
	{
		GWarn->BeginSlowTask(LOCTEXT("StatusBeginMaterialRessouces", "Importing requested ressource package"), true, false);
		ExportPackageToRequirements(Package, Type);
	}

	ResolveRequirements();
	GWarn->EndSlowTask();
}

FString T3DLevelParser::ExportFolderFor(EExportType::Type Type)
{
	FString Directory;
	switch (Type)
	{
	case EExportType::Material: Directory = TEXT("ExportedMaterials"); break;
	case EExportType::StaticMesh: Directory = TEXT("ExportedMeshes"); break;
	case EExportType::MaterialInstanceConstant: Directory = TEXT("ExportedMaterialInstances"); break;
	case EExportType::Texture2D: Directory = TEXT("ExportedTextures"); break;
	case EExportType::Texture2DInfo: Directory = TEXT("ExportedTexturesT3D"); break;
	default: Directory = TEXT("ExportedUnknowns"); break;
	}

	Directory = TmpPath / Directory;
	IFileManager::Get().MakeDirectory(*Directory, true);
	return Directory;
}

FString T3DLevelParser::RessourceTypeFor(EExportType::Type Type)
{
	switch (Type)
	{
	case EExportType::Material: return TEXT("Material");
	case EExportType::StaticMesh: return TEXT("StaticMesh");
	case EExportType::MaterialInstanceConstant: return TEXT("MaterialInstanceConstant");
	case EExportType::Texture2D: return TEXT("Texture2D");
	default: return TEXT("Unknow");
	}
}

void T3DLevelParser::ExportPackageToRequirements(const FString &Package, EExportType::Type Type)
{
	FString ExportFolder;
	if (ExportPackage(Package, Type, ExportFolder))
	{
		FString RessourceType = RessourceTypeFor(Type);

		TArray<FString> FileNames;
		IFileManager::Get().FindFiles(FileNames, *(ExportFolder / TEXT("*")), true, false);
		for (int32 NameIdx = 0; NameIdx < FileNames.Num(); NameIdx++)
		{
			const FString & Name = FileNames[NameIdx];
			if (Name.MatchesWildcard(TEXT("*.???")))
			{
				AddRequirement(FString::Printf(TEXT("%s'%s.%s'"), *RessourceType, *Package, *Name.LeftChop(4)), UObjectDelegate());
			}
		}
	}
}

bool T3DLevelParser::ExportPackage(const FString &Package, EExportType::Type Type, FString & ExportFolder)
{
	ExportFolder = ExportFolderFor(Type) / Package;

	if (!IFileManager::Get().DirectoryExists(*ExportFolder))
	{
		FString Command;
		switch (Type)
		{
		case EExportType::Material: Command = TEXT("Material T3D"); break;
		case EExportType::StaticMesh: Command = TEXT("StaticMesh OBJ"); break;
		case EExportType::MaterialInstanceConstant: Command = TEXT("MaterialInstanceConstant T3D"); break;
		case EExportType::Texture2D: Command = TEXT("Texture TGA"); break;
		case EExportType::Texture2DInfo: Command = TEXT("Texture T3D"); break;
		default: return false;
		}

		return RunUDK(FString::Printf(TEXT("batchexport %s %s %s"), *Package, *Command, *ExportFolder)) == 0;
	}

	return true;
}

void T3DLevelParser::ResolveRequirements()
{
	FAssetToolsModule& AssetToolsModule = FModuleManager::Get().LoadModuleChecked<FAssetToolsModule>("AssetTools");
	
	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportStaticMeshRequirements", "Exporting StaticMesh referenced assets"));
	ExportStaticMeshRequirements();

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportMaterialInstanceConstantAssets", "Exporting MaterialInstanceConstant assets"));
	ExportMaterialInstanceConstantAssets();

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportMaterialAssets", "Exporting ExportMaterial assets"));
	ExportMaterialAssets();

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportTextureAssets", "Exporting Texture assets"));
	ExportTextureAssets();

	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ExportStaticMeshAssets", "Exporting StaticMesh assets"));
	ExportStaticMeshAssets();
	
	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ImportAssets", "Importing Assets"));
	TArray<FString> AssetsPath;
	AssetsPath.Add(TmpPath / TEXT("UDK"));
	AssetToolsModule.Get().ImportAssets(AssetsPath, TEXT("/Game/"));
	
	GWarn->StatusUpdate(++StatusNumerator, StatusDenominator, LOCTEXT("ResolvingLinks", "Updating actors assets"));
	UTexture2D * DefaultTexture2D = FindObject<UTexture2D>(NULL, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();

		if (Requirement.Type == TEXT("StaticMesh"))
		{
			FString ObjectPath = FString::Printf(TEXT("/Game/UDK/%s/Meshes/%s.%s"), *Requirement.Package, *Requirement.Name, *Requirement.Name);
			UObject * Object = FindObject<UStaticMesh>(NULL, *ObjectPath);
			if (Object)
			{
				FixRequirement(Requirement, Object);
			}
		}
		else if (Requirement.Type.StartsWith(TEXT("Texture")))
		{
			FString ObjectPath = FString::Printf(TEXT("/Game/UDK/%s/Textures/%s.%s"), *Requirement.Package, *Requirement.Name, *Requirement.Name);
			UTexture2D * Texture2D = FindObject<UTexture2D>(NULL, *ObjectPath);
			if (!Texture2D)
			{
				UE_LOG(UDKImportPluginLog, Warning, TEXT("Missing requirements : %s"), *Requirement.Url);
				Texture2D = DefaultTexture2D;
			}
			FixRequirement(Requirement, Texture2D);
		}
	}

	// make sure that any static meshes, etc using this material will stop using the FMaterialResource of the original 
	// material, and will use the new FMaterialResource created when we make a new UMaterial in place
	FGlobalComponentReregisterContext RecreateComponents;

	// Compile Materials
	PostEditChangeFor(TEXT("Material"));
	PostEditChangeFor(TEXT("MaterialInstanceConstant"));
	PostEditChangeFor(TEXT("StaticMesh"));

	PrintMissingRequirements();
}

void T3DLevelParser::PostEditChangeFor(const FString &Type)
{
	for (auto Iter = FixedRequirements.CreateIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();
		if (Requirement.Type == Type)
		{
			if (Requirement.Name == TEXT("MI_StargateSupport_Base"))
			{
				UE_LOG(UDKImportPluginLog, Warning, TEXT("Test Me : %s"), *Requirement.Url);
			}
			UObject * Object = Iter.Value();
			if (Object)
			{
				Object->PostEditChange();
			}
		}
	}
}

void T3DLevelParser::ExportStaticMeshRequirements()
{
	int32 StaticMeshesParamsCount = 0;
	FString StaticMeshesParams = TEXT("run UDKPluginExport.ExportStaticMeshMaterials");
	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();

		if (Requirement.Type == TEXT("StaticMesh"))
		{
			StaticMeshesParams += TEXT(" ") + Requirement.OriginalUrl;
			++StaticMeshesParamsCount;
			if (StaticMeshesParamsCount >= 200)
			{
				ExportStaticMeshRequirements(StaticMeshesParams);
				StaticMeshesParamsCount = 0;
				StaticMeshesParams = TEXT("run UDKPluginExport.ExportStaticMeshMaterials");
			}
		}
	}

	if (StaticMeshesParamsCount > 0)
	{
		ExportStaticMeshRequirements(StaticMeshesParams);
	}
}

void T3DLevelParser::ExportStaticMeshRequirements(const FString &StaticMeshesParams)
{
	FString ExportStaticMeshMaterialsOutput;
	if (RunUDK(StaticMeshesParams, ExportStaticMeshMaterialsOutput) == 0)
	{
		ResetParser(ExportStaticMeshMaterialsOutput);
		while (NextLine())
		{
			if (Line.StartsWith(TEXT("ScriptLog: ")))
			{
				int32 StaticMeshUrlEndIndex = Line.Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromStart, 11);
				int32 MaterialIdxEndIndex = Line.Find(TEXT(" "), ESearchCase::CaseSensitive, ESearchDir::FromStart, StaticMeshUrlEndIndex + 1);
				if (StaticMeshUrlEndIndex != -1 && MaterialIdxEndIndex != -1)
				{
					FString StaticMeshUrl = Line.Mid(11, StaticMeshUrlEndIndex - 11);
					int32 MaterialIdx = FCString::Atoi(*Line.Mid(StaticMeshUrlEndIndex + 1, MaterialIdxEndIndex - StaticMeshUrlEndIndex - 1));
					FString MaterialUrl = Line.Mid(MaterialIdxEndIndex + 1);
					AddRequirement(MaterialUrl, UObjectDelegate::CreateRaw(this, &T3DLevelParser::SetStaticMeshMaterial, StaticMeshUrl, MaterialIdx));
				}
			}
		}
	}
}

void T3DLevelParser::ExportMaterialInstanceConstantAssets()
{
	bool bRequiresAnotherLoop = false;

	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();

		if (Requirement.Type == TEXT("MaterialInstanceConstant"))
		{
			FString ExportFolder;
			FString FileName = Requirement.Name + TEXT(".T3D");
			ExportPackage(Requirement.Package, EExportType::MaterialInstanceConstant, ExportFolder);

			FString ObjectPath = FString::Printf(TEXT("/Game/UDK/%s/MaterialInstances/%s.%s"), *Requirement.Package, *Requirement.Name, *Requirement.Name);
			UMaterialInstanceConstant * MaterialInstanceConstant = LoadObject<UMaterialInstanceConstant>(NULL, *ObjectPath, NULL, LOAD_NoWarn | LOAD_Quiet);
			if (!MaterialInstanceConstant)
			{
				T3DMaterialInstanceConstantParser MaterialInstanceConstantParser(this, Requirement.Package);
				MaterialInstanceConstant = MaterialInstanceConstantParser.ImportT3DFile(ExportFolder / FileName);
			}

			if (MaterialInstanceConstant)
			{
				bRequiresAnotherLoop = true;
				FixRequirement(Requirement, MaterialInstanceConstant);
			}
			else
			{
				UE_LOG(UDKImportPluginLog, Warning, TEXT("Unable to import : %s"), *Requirement.Url);
			}
		}
	}

	if (bRequiresAnotherLoop)
	{
		ExportMaterialInstanceConstantAssets();
	}
}

void T3DLevelParser::ExportMaterialAssets()
{
	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();

		if (Requirement.Type == TEXT("Material"))
		{
			FString ExportFolder;
			FString FileName = Requirement.Name + TEXT(".T3D");
			ExportPackage(Requirement.Package, EExportType::Material, ExportFolder);
				
			FString ObjectPath = FString::Printf(TEXT("/Game/UDK/%s/Materials/%s.%s"), *Requirement.Package, *Requirement.Name, *Requirement.Name);
			UMaterial * Material = LoadObject<UMaterial>(NULL, *ObjectPath, NULL, LOAD_NoWarn | LOAD_Quiet);
			if (!Material)
			{
				T3DMaterialParser MaterialParser(this, Requirement.Package);
				Material = MaterialParser.ImportMaterialT3DFile(ExportFolder / FileName);
			}

			if (Material)
			{
				FixRequirement(Requirement, Material);
			}
			else
			{
				UE_LOG(UDKImportPluginLog, Warning, TEXT("Unable to import : %s"), *Requirement.Url);
			}
		}
	}
}

void T3DLevelParser::ExportTextureAssets()
{
	IFileManager & FileManager = IFileManager::Get();

	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();

		if (Requirement.Type.StartsWith(TEXT("Texture")))
		{
			FString ExportFolder;
			FString ImportFolder = TmpPath / TEXT("UDK") / Requirement.Package / TEXT("Textures");
			FString FileName = Requirement.Name + TEXT(".TGA");
			ExportPackage(Requirement.Package, EExportType::Texture2D, ExportFolder);

			FileManager.MakeDirectory(*ImportFolder, true);
			if (FileManager.FileSize(*(ExportFolder / FileName)) > 0)
			{
				FileManager.Copy(*(ImportFolder / FileName), *(ExportFolder / FileName));
			}
		}
	}
}

void T3DLevelParser::ExportStaticMeshAssets()
{
	FString ExportFolder, ImportFolder, FileNameOBJ, FileNameFBX;
	IFileManager & FileManager = IFileManager::Get();

	FileManager.MakeDirectory(*(TmpPath / TEXT("ExportedMeshes")), true);

	for (auto Iter = Requirements.CreateConstIterator(); Iter; ++Iter)
	{
		const FRequirement &Requirement = Iter.Key();

		if (Requirement.Type == TEXT("StaticMesh"))
		{
			FString ExportFolder;
			FString ImportFolder = TmpPath / TEXT("UDK") / Requirement.Package / TEXT("Meshes");
			FString FileNameOBJ = Requirement.Name + TEXT(".OBJ");
			FString FileNameFBX = Requirement.Name + TEXT(".FBX");
			ExportPackage(Requirement.Package, EExportType::StaticMesh, ExportFolder);

			FileManager.MakeDirectory(*ImportFolder, true);
			if (FileManager.FileSize(*(ExportFolder / FileNameFBX)) > 0)
			{
				FileManager.Copy(*(ImportFolder / FileNameFBX), *(ExportFolder / FileNameFBX));
			}
			else if (FileManager.FileSize(*(ExportFolder / FileNameOBJ)) > 0)
			{
				ConvertOBJToFBX(ExportFolder / FileNameOBJ, ImportFolder / FileNameFBX);
			}
		}
	}
}

void T3DLevelParser::ImportLevel()
{
	FString Class;

	ensure(NextLine());
	ensure(Line.Equals(TEXT("Begin Object Class=Level Name=PersistentLevel")));

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class))
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
}

void T3DLevelParser::ImportBrush()
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
		else if (IsActorLocation(Brush) || IsActorProperty(Brush))
		{
			continue;
		}
		else if (Line.StartsWith(TEXT("Begin "), ESearchCase::CaseSensitive))
		{
			JumpToEnd();
		}
	}
	
	Model->Modify();
	Model->BuildBound();

	Brush->BrushComponent->Brush = Brush->Brush;
	Brush->PostEditImport();
	Brush->PostEditChange();
}

void T3DLevelParser::ImportPolyList(UPolys * Polys)
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
				AddRequirement(FString::Printf(TEXT("Material'%s'"), *Texture), UObjectDelegate::CreateRaw(this, &T3DLevelParser::SetPolygonTexture, Polys, Polys->Element.Num()));
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

void T3DLevelParser::ImportPointLight()
{
	FString Value, Class;
	APointLight* PointLight = SpawnActor<APointLight>();

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class))
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
		else if (IsActorLocation(PointLight) || IsActorRotation(PointLight) || IsActorProperty(PointLight))
		{
			continue;
		}
	}
	PointLight->PostEditChange();
}

void T3DLevelParser::ImportSpotLight()
{
	FVector DrawScale3D(1.0,1.0,1.0);
	FRotator Rotator(0.0, 0.0, 0.0);
	FString Value, Class, Name;
	ASpotLight* SpotLight = SpawnActor<ASpotLight>();

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class))
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
		else if (IsActorLocation(SpotLight) || IsActorProperty(SpotLight))
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
	SpotLight->PostEditChange();
}

void T3DLevelParser::ImportStaticMeshActor()
{
	FString Value, Class;
	FVector PrePivot;
	bool bPrePivotFound = false;
	AStaticMeshActor * StaticMeshActor = SpawnActor<AStaticMeshActor>();
	
	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(Class))
		{
			if (Class.Equals(TEXT("StaticMeshComponent")))
			{
				while (NextLine() && !IsEndObject())
				{
					if (GetProperty(TEXT("StaticMesh="), Value))
					{
						AddRequirement(Value, UObjectDelegate::CreateRaw(this, &T3DLevelParser::SetStaticMesh, StaticMeshActor->StaticMeshComponent.Get()));
					}
				}
			}
			else
			{
				JumpToEnd();
			}
		}
		else if (IsActorLocation(StaticMeshActor) || IsActorRotation(StaticMeshActor) || IsActorScale(StaticMeshActor) || IsActorProperty(StaticMeshActor))
		{
			continue;
		}
		else if (GetProperty(TEXT("PrePivot="), Value))
		{
			ensure(PrePivot.InitFromString(Value));
			bPrePivotFound = true;
		}
	}

	if (bPrePivotFound)
	{
		PrePivot = StaticMeshActor->GetActorRotation().RotateVector(PrePivot);
		StaticMeshActor->SetActorLocation(StaticMeshActor->GetActorLocation() - PrePivot);
	}
	StaticMeshActor->PostEditChange();
}

USoundCue * T3DLevelParser::ImportSoundCue()
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
			AddRequirement(Value, UObjectDelegate::CreateRaw(this, &T3DLevelParser::SetSoundCueFirstNode, SoundCue));
		}
	}

	return SoundCue;
}

void T3DLevelParser::SetPolygonTexture(UObject * Object, UPolys * Polys, int32 index)
{
	Polys->Element[index].Material = Cast<UMaterialInterface>(Object);
}

void T3DLevelParser::SetStaticMesh(UObject * Object, UStaticMeshComponent * StaticMeshComponent)
{
	UProperty* ChangedProperty = FindField<UProperty>(UStaticMeshComponent::StaticClass(), "StaticMesh");
	UStaticMesh * StaticMesh = Cast<UStaticMesh>(Object);
	StaticMeshComponent->PreEditChange(ChangedProperty);

	StaticMeshComponent->StaticMesh = StaticMesh;

	FPropertyChangedEvent PropertyChangedEvent(ChangedProperty);
	StaticMeshComponent->PostEditChangeProperty(PropertyChangedEvent);
}

void T3DLevelParser::SetSoundCueFirstNode(UObject * Object, USoundCue * SoundCue)
{
	SoundCue->FirstNode = Cast<USoundNode>(Object);
}

void T3DLevelParser::SetStaticMeshMaterial(UObject * Material, FString StaticMeshUrl, int32 MaterialIdx)
{
	AddRequirement(StaticMeshUrl, UObjectDelegate::CreateRaw(this, &T3DLevelParser::SetStaticMeshMaterialResolved, Material, MaterialIdx));
}

void T3DLevelParser::SetStaticMeshMaterialResolved(UObject * Object, UObject * Material, int32 MaterialIdx)
{
	UStaticMesh * StaticMesh = Cast<UStaticMesh>(Object);

	check(StaticMesh->RenderData);
	FMeshSectionInfo Info = StaticMesh->SectionInfoMap.Get(0, MaterialIdx);

	if (MaterialIdx >= StaticMesh->Materials.Num())
		StaticMesh->Materials.SetNum(MaterialIdx + 1);
	Info.MaterialIndex = MaterialIdx;
	StaticMesh->SectionInfoMap.Set(0, MaterialIdx, Info);
	StaticMesh->Materials[MaterialIdx] = Cast<UMaterialInterface>(Material);

	StaticMesh->Modify();
	StaticMesh->PostEditChange();
}

void T3DLevelParser::SetTexture(UObject * Object, UMaterialExpressionTextureBase * MaterialExpression)
{
	UTexture * Texture = Cast<UTexture>(Object);
	MaterialExpression->Texture = Texture;
	switch (Texture->CompressionSettings)
	{
	case TC_Normalmap:
		MaterialExpression->SamplerType = SAMPLERTYPE_Normal;
		break;
	case TC_Grayscale:
		MaterialExpression->SamplerType = SAMPLERTYPE_Grayscale;
		break;
	case TC_Masks:
		MaterialExpression->SamplerType = SAMPLERTYPE_Masks;
		break;
	case TC_Alpha:
		MaterialExpression->SamplerType = SAMPLERTYPE_Alpha;
		break;
	default:
		MaterialExpression->SamplerType = SAMPLERTYPE_Color;
		break;
	}
}

void T3DLevelParser::SetParent(UObject * Object, UMaterialInstanceConstant * MaterialInstanceConstant)
{
	UMaterialInterface * MaterialInterface = Cast<UMaterialInterface>(Object);
	MaterialInstanceConstant->Parent = MaterialInterface;
}

void T3DLevelParser::SetTextureParameterValue(UObject * Object, UMaterialInstanceConstant * MaterialInstanceConstant, int32 ParameterIndex)
{
	UTexture * Texture = Cast<UTexture>(Object);
	MaterialInstanceConstant->TextureParameterValues[ParameterIndex].ParameterValue = Texture;
}
