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
	const FString UdkPath = SUDKPath.Get()->GetText().ToString();
	const FString TmpPath = STmpPath.Get()->GetText().ToString();
	const FString Level = SLevel.Get()->GetText().ToString();

	T3DParser parser(UdkPath, TmpPath);
	parser.ImportLevel(Level);

	return FReply::Handled();
}
