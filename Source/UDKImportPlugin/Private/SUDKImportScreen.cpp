#include "UDKImportPluginPrivatePCH.h"
#include "SUDKImportScreen.h"
#include "T3DLevelParser.h"

#define LOCTEXT_NAMESPACE "UDKImportScreen"

DEFINE_LOG_CATEGORY(LogUDKImportPlugin);

FText EUDKImportMode::ToName(const Type ExportType)
{
	switch (ExportType)
	{
	case Map: return LOCTEXT("UDKImportMode_Name_LVL", "Level");
	case StaticMesh: return LOCTEXT("UDKImportMode_Name_SM", "Static Mesh");
	case Material: return LOCTEXT("UDKImportMode_Name_M", "Material");
	case MaterialInstanceConstant: return LOCTEXT("UDKImportMode_Name_MIC", "Material Instance Constant");

	default: return LOCTEXT("InvalidOrMax", "InvalidOrMax");
	}
}

FText EUDKImportMode::ToDescription(const Type ExportType)
{
	switch (ExportType)
	{
	case Map: return LOCTEXT("UDKImportMode_Desc_LVL", "Import the requested map");
	case StaticMesh: return LOCTEXT("UDKImportMode_Desc_SM", "Import StaticMeshes (whole package or just one)");
	case Material: return LOCTEXT("UDKImportMode_Desc_M", "Import Materials (whole package or just one)");
	case MaterialInstanceConstant: return LOCTEXT("UDKImportMode_Desc_MIC", "Import MaterialInstances (whole package or just one)");

	default: return LOCTEXT("InvalidOrMax", "InvalidOrMax");
	}
}

void SUDKImportScreen::ExportType_OnSelectionChanged(TSharedPtr<EUDKImportMode::Type> NewExportMode, ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		ExportMode = *NewExportMode;
	}
}

TSharedRef<SWidget> SUDKImportScreen::ExportType_OnGenerateWidget(TSharedPtr<EUDKImportMode::Type> InExportMode) const
{
	return SNew(STextBlock)
		.Text(EUDKImportMode::ToName(*InExportMode))
		.ToolTipText(EUDKImportMode::ToDescription(*InExportMode));
}

FText SUDKImportScreen::ExportType_GetSelectedText() const
{
	return EUDKImportMode::ToName(ExportMode);
}

SUDKImportScreen::SUDKImportScreen()
	: ExportMode(EUDKImportMode::Map)
{

}

SUDKImportScreen::~SUDKImportScreen()
{

}

void SUDKImportScreen::Construct(const FArguments& Args)
{	
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(1.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("UDKPathLabel", "Path to UDK"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("ExportType", "Exportation mode"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("Reference", "Reference"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(LOCTEXT("TmpDirLabel", "Temporary Directory"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			.VAlign(VAlign_Center)
		]
		+ SHorizontalBox::Slot()
		.FillWidth(2.0f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SAssignNew(SUDKPath, SEditableTextBox)
				.Text(FText::FromString(TEXT("C:/UDK/UDK-2014-02")))
				.ToolTipText(LOCTEXT("UDKPath", "Path to the UDK directory(ex: \"C:/UDK/UDK-2014-02\")"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SAssignNew(ExportTypeComboBox, SComboBox< TSharedPtr<EUDKImportMode::Type> >)
				.OptionsSource(&ExportTypeOptionsSource)
				.OnSelectionChanged(this, &SUDKImportScreen::ExportType_OnSelectionChanged)
				.OnGenerateWidget(this, &SUDKImportScreen::ExportType_OnGenerateWidget)
				[
					SNew(STextBlock)
					.Text(this, &SUDKImportScreen::ExportType_GetSelectedText)
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SAssignNew(SLevel, SEditableTextBox)
				.Text(FText::FromString(TEXT("MyPackage.MyRessouce")))
				.ToolTipText(LOCTEXT("MyPackage.MyRessouce", "Reference to the ressource or package (eg: MyLevel or MyPackage.MyRessource)"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SAssignNew(STmpPath, SEditableTextBox)
				.Text(FText::FromString(TEXT("C:/UDK/TemporaryFolder")))
				.ToolTipText(LOCTEXT("ExportTmpFolder", "An existing temporary folder to use for the UDK exportation"))
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SNew(SButton)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				.Text(LOCTEXT("Run", "Run"))
				.OnClicked(this, &SUDKImportScreen::OnRun)
			]
		]
	];

	ExportTypeOptionsSource.Reset(5);
	ExportTypeOptionsSource.Add(MakeShareable(new EUDKImportMode::Type(EUDKImportMode::Map)));
	ExportTypeOptionsSource.Add(MakeShareable(new EUDKImportMode::Type(EUDKImportMode::StaticMesh)));
	ExportTypeOptionsSource.Add(MakeShareable(new EUDKImportMode::Type(EUDKImportMode::Material)));
	ExportTypeOptionsSource.Add(MakeShareable(new EUDKImportMode::Type(EUDKImportMode::MaterialInstanceConstant)));
}

FReply SUDKImportScreen::OnRun()
{
	const FString UdkPath = SUDKPath.Get()->GetText().ToString();
	const FString TmpPath = STmpPath.Get()->GetText().ToString();
	const FString Ressource = SLevel.Get()->GetText().ToString();

	switch (ExportMode)
	{
	case EUDKImportMode::Map:
	{
		T3DLevelParser Parser(UdkPath, TmpPath);
		Parser.ImportLevel(Ressource);
		break;
	}
	case EUDKImportMode::StaticMesh:
	{
		T3DLevelParser Parser(UdkPath, TmpPath);
		Parser.ImportStaticMesh(Ressource);
		break;
	}
	case EUDKImportMode::Material:
	{
		T3DLevelParser Parser(UdkPath, TmpPath);
		Parser.ImportMaterial(Ressource);
		break;
	}
	case EUDKImportMode::MaterialInstanceConstant:
	{
		T3DLevelParser Parser(UdkPath, TmpPath);
		Parser.ImportMaterialInstanceConstant(Ressource);
		break;
	}
	}

	return FReply::Handled();
}
