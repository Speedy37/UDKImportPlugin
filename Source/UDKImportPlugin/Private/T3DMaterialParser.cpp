#include "UDKImportPluginPrivatePCH.h"
#include "T3DMaterialParser.h"
#include "T3DLevelParser.h"

T3DMaterialParser::T3DMaterialParser(T3DLevelParser * ParentParser, const FString &Package) : T3DParser(ParentParser->UdkPath, ParentParser->TmpPath)
{
	this->LevelParser = ParentParser;
	this->Package = Package;
	this->Material = NULL;
}

UMaterial* T3DMaterialParser::ImportMaterialT3DFile(const FString &FileName)
{
	FString MaterialT3D;
	if (FFileHelper::LoadFileToString(MaterialT3D, *FileName))
	{
		ResetParser(MaterialT3D);
		MaterialT3D.Empty();
		return ImportMaterial();
	}

	return NULL;
}

UMaterial*  T3DMaterialParser::ImportMaterial()
{
	FString ClassName, Name, Value;
	UClass * Class;

	ensure(NextLine());
	ensure(IsBeginObject(ClassName));
	ensure(ClassName == TEXT("Material"));
	ensure(GetOneValueAfter(TEXT(" Name="), Name));

	FString BasePackageName = TEXT("/Game/UDK/Materials");
	BasePackageName /= Package;

	FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
	UMaterialFactoryNew* MaterialFactory = ConstructObject<UMaterialFactoryNew>(UMaterialFactoryNew::StaticClass());
	Material = (UMaterial*)AssetToolsModule.Get().CreateAsset(Name, BasePackageName, UMaterial::StaticClass(), MaterialFactory);
	if (Material == NULL)
	{
		return NULL;
	}

	Material->Modify();

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(ClassName))
		{
			Class = (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *ClassName, true);
			if (Class)
			{
				ensure(GetOneValueAfter(TEXT(" Name="), Name));
				UMaterialExpression* MaterialExpression = ImportMaterialExpression(Class);
				UMaterialExpressionComment * MaterialExpressionComment = Cast<UMaterialExpressionComment>(MaterialExpression);
				if (MaterialExpressionComment)
				{
					Material->EditorComments.Add(MaterialExpressionComment);
					MaterialExpressionComment->MaterialExpressionEditorX -= MaterialExpressionComment->SizeX;
					FixRequirement(FString::Printf(TEXT("%s'%s'"), *ClassName, *Name), MaterialExpression);
				}
				else if (MaterialExpression)
				{
					Material->Expressions.Add(MaterialExpression);
					FixRequirement(FString::Printf(TEXT("%s'%s'"), *ClassName, *Name), MaterialExpression);
				}
			}
			else
			{
				JumpToEnd();
			}
		}
		else if (GetProperty(TEXT("DiffuseColor="), Value))
		{
			ImportExpression(&Material->BaseColor);
		}
		else if (GetProperty(TEXT("SpecularColor="), Value))
		{
			ImportExpression(&Material->Specular);
		}
		else if (GetProperty(TEXT("SpecularPower="), Value))
		{
			// TODO
		}
		else if (GetProperty(TEXT("Normal="), Value))
		{
			ImportExpression(&Material->Normal);
		}
		else if (GetProperty(TEXT("EmissiveColor="), Value))
		{
			ImportExpression(&Material->EmissiveColor);
		}
		else if (GetProperty(TEXT("Opacity="), Value))
		{
			ImportExpression(&Material->Opacity);
		}
	}

	CheckMaterialExpressionTextureNormal(Material->Normal.Expression);

	PrintMissingRequirements();

	return Material;
}

void T3DMaterialParser::CheckMaterialExpressionTextureNormal(UMaterialExpression * MaterialExpression)
{
	if (!MaterialExpression)
	{
		return;
	}

	if (MaterialExpression->IsA<UMaterialExpressionTextureBase>())
	{
		((UMaterialExpressionTextureBase*)MaterialExpression)->SamplerType = SAMPLERTYPE_Normal;
	}

	uint32 InputIndex = 0;
	while (FExpressionInput* PtrInput = MaterialExpression->GetInput(InputIndex++))
	{
		CheckMaterialExpressionTextureNormal(PtrInput->Expression);
	}
}

UMaterialExpression* T3DMaterialParser::ImportMaterialExpression(UClass * Class)
{
	if (!Class->IsChildOf(UMaterialExpression::StaticClass()))
		return NULL;
	UMaterialExpression* MaterialExpression = ConstructObject<UMaterialExpression>(Class, Material);

	FString Value, Name, PropertyName, Type, PackageName;
	while (NextLine() && IgnoreSubs() && !IsEndObject())
	{
		if (GetProperty(TEXT("Texture="), Value))
		{
			FRequirement Requirement;
			if (ParseRessourceUrl(Value, Requirement))
			{
				LevelParser->AddRequirement(Requirement, UObjectDelegate::CreateRaw(LevelParser, &T3DLevelParser::SetTexture, (UMaterialExpressionTextureBase*)MaterialExpression));
			}
			else
			{
				UE_LOG(UDKImportPluginLog, Warning, TEXT("Unable to parse ressource url : %s"), *Value);
			}
		}
		else if (IsProperty(PropertyName, Value) 
			&& PropertyName != TEXT("Material")
			&& PropertyName != TEXT("ExpressionGUID")
			&& PropertyName != TEXT("ObjectArchetype"))
		{
			UProperty* Property = FindField<UProperty>(Class, *PropertyName);
			UStructProperty * StructProperty = Cast<UStructProperty>(Property);
			if (StructProperty && StructProperty->Struct->GetName() == TEXT("ExpressionInput"))
			{
				FExpressionInput * ExpressionInput = Property->ContainerPtrToValuePtr<FExpressionInput>(MaterialExpression);
				ImportExpression(ExpressionInput);
			}
			else if (Property)
			{
				Property->ImportText(*Value, Property->ContainerPtrToValuePtr<uint8>(MaterialExpression), 0, MaterialExpression);
			}
		}
	}
	MaterialExpression->Material = Material;
	MaterialExpression->MaterialExpressionEditorX = -MaterialExpression->MaterialExpressionEditorX;

	return MaterialExpression;
}

void T3DMaterialParser::ImportExpression(FExpressionInput * ExpressionInput)
{
	FString Value;
	if (GetOneValueAfter(TEXT("Expression="), Value))
		AddRequirement(Value, UObjectDelegate::CreateRaw(this, &T3DMaterialParser::SetExpression, ExpressionInput));
	if (GetOneValueAfter(TEXT("Mask="), Value))
		ExpressionInput->Mask = FCString::Atoi(*Value);
	if (GetOneValueAfter(TEXT("MaskR="), Value))
		ExpressionInput->MaskR = FCString::Atoi(*Value);
	if (GetOneValueAfter(TEXT("MaskG="), Value))
		ExpressionInput->MaskG = FCString::Atoi(*Value);
	if (GetOneValueAfter(TEXT("MaskB="), Value))
		ExpressionInput->MaskB = FCString::Atoi(*Value);
}

void T3DMaterialParser::SetExpression(UObject * Object, FExpressionInput * ExpressionInput)
{
	UMaterialExpression * Expression = Cast<UMaterialExpression>(Object);
	ExpressionInput->Expression = Expression;
}
