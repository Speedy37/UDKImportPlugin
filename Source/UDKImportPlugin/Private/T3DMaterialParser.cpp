#include "UDKImportPluginPrivatePCH.h"
#include "T3DMaterialParser.h"
#include "T3DLevelParser.h"

T3DMaterialParser::T3DMaterialParser(T3DLevelParser * ParentParser) : T3DParser(ParentParser->UdkPath, ParentParser->TmpPath)
{
	LevelParser = ParentParser;
}

UMaterialExpression* T3DMaterialParser::BeginMaterialExpression(UClass * Class)
{
	if (!Class->IsChildOf(UMaterialExpression::StaticClass()))
		return NULL;
	UMaterialExpression* CreatedObject = ConstructObject<UMaterialExpression>(Class, Material);
	return CreatedObject;
}

void T3DMaterialParser::EndMaterialExpression()
{

}

void T3DMaterialParser::ImportMaterialT3DFile(const FString &FileName, const FString &Package)
{
	FString MaterialT3D;
	if (FFileHelper::LoadFileToString(MaterialT3D, *FileName))
	{
		ResetParser(MaterialT3D);
		MaterialT3D.Empty();
		ImportMaterial(Package);
	}
}

void T3DMaterialParser::ImportMaterial(const FString &Package)
{
	FString ClassName, Name, Value;
	UClass * Class;
	// 
	// ParseObject<UClass>(Str, TEXT("CLASS="), ObjClass, ANY_PACKAGE)
	// ParseObject<UClass>(Str, TEXT("CLASS="), ObjClass, ANY_PACKAGE)
	// ParseObject( Str, TEXT("CLASS="), UClass::StaticClass(), (UObject*&)ObjClass, ANY_PACKAGE, NULL);
	// 
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
		return;
	}

	Material->Modify();

	LevelParser->FixRequirement(FString::Printf(TEXT("%s'%s'"), *ClassName, *Name), Material);

	while (NextLine() && !IsEndObject())
	{
		if (IsBeginObject(ClassName))
		{
			Class = (UClass*)StaticFindObject(UClass::StaticClass(), ANY_PACKAGE, *ClassName, true);
			if (Class)
			{
				ensure(GetOneValueAfter(TEXT(" Name="), Name));
				UMaterialExpression* MaterialExpression = ImportMaterialExpression(Class);
				if (MaterialExpression)
				{
					Material->AddExpressionParameter(MaterialExpression);
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
			ImportExpression(&Material->SpecularColor);
		}
		else if (GetProperty(TEXT("SpecularPower="), Value))
		{
			ImportExpression(&Material->Specular);
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
}

UMaterialExpression* T3DMaterialParser::ImportMaterialExpression(UClass * Class)
{
	UMaterialExpression* MaterialExpression = BeginMaterialExpression(Class);
	if (!MaterialExpression)
		return NULL;

	FString Value, Name, PropertyName;
	while (NextLine() && IgnoreSubs() && !IsEndObject())
	{
		if (GetProperty(TEXT("Material="), Value))
		{
			MaterialExpression->Material = Material;
		}
		else if (GetProperty(TEXT("Texture="), Value))
		{
			LevelParser->AddRequirement(Value, UObjectDelegate::CreateRaw(LevelParser, &T3DLevelParser::SetTexture, (UMaterialExpressionTextureBase*)MaterialExpression));
		}
		else if (IsProperty(PropertyName, Value))
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
	EndMaterialExpression();

	return MaterialExpression;
}

void T3DMaterialParser::ImportExpression(FExpressionInput * ExpressionInput)
{
	FString Value;
	if (GetOneValueAfter(TEXT("Expression="), Value))
		AddRequirement(Value, UObjectDelegate::CreateRaw(this, &T3DMaterialParser::SetExpression, ExpressionInput));
	else if (GetOneValueAfter(TEXT("Mask="), Value))
		ExpressionInput->Mask = FCString::Atoi(*Value);
	else if (GetOneValueAfter(TEXT("MaskR="), Value))
		ExpressionInput->MaskR = FCString::Atoi(*Value);
	else if (GetOneValueAfter(TEXT("MaskG="), Value))
		ExpressionInput->MaskG = FCString::Atoi(*Value);
	else if (GetOneValueAfter(TEXT("MaskB="), Value))
		ExpressionInput->MaskB = FCString::Atoi(*Value);
}

void T3DMaterialParser::SetExpression(UObject * Object, FExpressionInput * ExpressionInput)
{
	UMaterialExpression * Expression = Cast<UMaterialExpression>(Object);
	ExpressionInput->Expression = Expression;
}
