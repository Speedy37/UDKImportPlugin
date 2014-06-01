#include "UDKImportPluginPrivatePCH.h"
#include "SUDKImportScreen.h"
#include "T3DParser.h"

#define LOCTEXT_NAMESPACE "UDKImportScreen"

DEFINE_LOG_CATEGORY(LogUDKImportPlugin);

void SUDKImportScreen::Construct(const FArguments& Args)
{
	SUDKPath = SNew(SEditableText);
	SLevel = SNew(SEditableText);
	STmpPath = SNew(SEditableText);

	ChildSlot
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			[
				SUDKPath.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			[
				SLevel.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			[
				STmpPath.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.VAlign(VAlign_Top)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Run", "Run"))
				.OnClicked(this, &SUDKImportScreen::OnRun)
			]
		];

}

FReply SUDKImportScreen::OnRun()
{
	const FString TmpPath = STmpPath.Get()->GetText().ToString();
	const FString CommandLine = FString::Printf(TEXT("batchexport %s T3D %s"), *(SLevel.Get()->GetText().ToString()), *TmpPath);

	if (RunUDK(CommandLine) == 0)
	{
		FString UdkLevelT3D;
		if (FFileHelper::LoadFileToString(UdkLevelT3D, *(TmpPath / TEXT("PersistentLevel.T3D"))))
		{
			T3DParser parser = UdkLevelT3D;
			parser.ImportLevel();
		}
	}

	return FReply::Handled();
}

int32 SUDKImportScreen::RunUDK(const FString &CommandLine)
{
	int32 exitCode = -1;
	FProcHandle ProcessHandle = FGenericPlatformProcess::CreateProc(*(SUDKPath.Get()->GetText().ToString() / TEXT("Binaries/Win32/UDK.exe")), *CommandLine, false, false, false, NULL, 0, NULL, NULL);

	if (ProcessHandle.IsValid())
	{
		FGenericPlatformProcess::WaitForProc(ProcessHandle);
		FGenericPlatformProcess::GetProcReturnCode(ProcessHandle, &exitCode);
	}

	return exitCode;
}

void SUDKImportScreen::ImportMaterial(const FString &UdkMaterialT3D)
{
	FString MaterialFullName = TEXT("Test");
	FString BasePackageName = TEXT("/Game/UDKImport");
	FString ObjectPath = BasePackageName + TEXT(".") + MaterialFullName;
	
	UMaterial* Material = LoadObject<UMaterial>(NULL, *ObjectPath);
	if (Material == NULL)
	{
		// create an unreal material asset
		UMaterialFactoryNew* MaterialFactory = ConstructObject<UMaterialFactoryNew>(UMaterialFactoryNew::StaticClass());

		const FString Suffix(TEXT(""));
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
		FString FinalPackageName;
		AssetToolsModule.Get().CreateUniqueAssetName(BasePackageName, Suffix, FinalPackageName, MaterialFullName);

		Material = (UMaterial*)AssetToolsModule.Get().CreateAsset(MaterialFullName, FinalPackageName, UMaterial::StaticClass(), MaterialFactory);
	}
	
	UMaterialExpressionComment *Comment = ConstructObject<UMaterialExpressionComment>(UMaterialExpressionComment::StaticClass(), Material, NAME_None, RF_Transactional);
	Comment->SizeX = 836;
	Comment->SizeY = 474;
	Comment->MaterialExpressionEditorX = 0;
	Comment->MaterialExpressionEditorY = 0;
	Comment->Text = TEXT("Test");
	Material->EditorComments.Add(Comment);
}
