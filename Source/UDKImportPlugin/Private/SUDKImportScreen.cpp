#include "UDKImportPluginPrivatePCH.h"
#include "SUDKImportScreen.h"
#include "T3DLevelParser.h"

#define LOCTEXT_NAMESPACE "UDKImportScreen"

DEFINE_LOG_CATEGORY(LogUDKImportPlugin);

void SUDKImportScreen::Construct(const FArguments& Args)
{
	SUDKPath = SNew(SEditableTextBox)
		.Text(FText::FromString(TEXT("C:/UDK/UDK-2014-02")))
		.ToolTipText(LOCTEXT("UDKPath", "Path to the UDK directory(ex: \"C:/UDK/UDK-2014-02\")"));
	SLevel = SNew(SEditableTextBox)
		.Text(FText::FromString(TEXT("MyLevel")))
		.ToolTipText(LOCTEXT("MapName", "Name of the map to export"));
	STmpPath = SNew(SEditableTextBox)
		.Text(FText::FromString(TEXT("C:/UDK/TemporaryFolder")))
		.ToolTipText(LOCTEXT("ExportTmpFolder", "An existing temporary folder to use for the UDK exportation"));

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
				.Text(LOCTEXT("MapNameLabel", "Map Name"))
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
				SUDKPath.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				SLevel.ToSharedRef()
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.Padding(2.0f)
			[
				STmpPath.ToSharedRef()
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
}

FReply SUDKImportScreen::OnRun()
{
	const FString UdkPath = SUDKPath.Get()->GetText().ToString();
	const FString TmpPath = STmpPath.Get()->GetText().ToString();
	const FString Level = SLevel.Get()->GetText().ToString();

	T3DLevelParser Parser(UdkPath, TmpPath);
	Parser.ImportLevel(Level);

	return FReply::Handled();
}
