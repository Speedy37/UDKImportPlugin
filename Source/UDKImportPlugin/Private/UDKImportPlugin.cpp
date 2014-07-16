#include "UDKImportPluginPrivatePCH.h"
#include "SUDKImportScreen.h"

#define LOCTEXT_NAMESPACE "UDKImportPlugin"

class FUDKImportPlugin : public IUDKImportPlugin
{
private:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** Add the menu extension for summoning the UDK Import tools */
	void AddSummonUDKImportMenuExtension(FMenuBuilder& MenuBuilder);

	/** Summon UDK Import page to front */
	void SummonUDKImport();

	/** ID name for the plugins editor major tab */
	static const FName UDKImportPluginTabName;

	/** The extender to pass to the level editor to extend it's window menu */
	TSharedPtr<FExtender> MainMenuExtender;
};

IMPLEMENT_MODULE(FUDKImportPlugin, UDKImportPlugin)

const FName FUDKImportPlugin::UDKImportPluginTabName(TEXT("UDKImportPlugin"));

void FUDKImportPlugin::StartupModule()
{
	// This code can run with content commandlets. Slate is not initialized with commandlets and the below code will fail.
	if (!IsRunningCommandlet())
	{
		// Add menu option for UDK import
		MainMenuExtender = MakeShareable(new FExtender);
		MainMenuExtender->AddMenuExtension("HelpBrowse", EExtensionHook::After, NULL, FMenuExtensionDelegate::CreateRaw(this, &FUDKImportPlugin::AddSummonUDKImportMenuExtension));
		FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
		LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MainMenuExtender);
	}
}

void FUDKImportPlugin::AddSummonUDKImportMenuExtension(FMenuBuilder& MenuBuilder)
{
	MenuBuilder.BeginSection("UDKImport", LOCTEXT("UDKImportLabel", "UDK Import"));
	MenuBuilder.AddMenuEntry(
		LOCTEXT("UDKImportMenuEntryTitle", "UDK Import"),
		LOCTEXT("UDKImportMenuEntryToolTip", "Opens up a tool for importing UDK maps."),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.Tutorials"),
		FUIAction(FExecuteAction::CreateRaw(this, &FUDKImportPlugin::SummonUDKImport)));
	MenuBuilder.EndSection();
}

void FUDKImportPlugin::SummonUDKImport()
{
	const FText UDKImportWindowTitle = LOCTEXT("UDKImportWindowTitle", "UDK Map importation tool");

	TSharedPtr<SWindow> UDKImportWindow =
		SNew(SWindow)
		.Title(UDKImportWindowTitle)
		.ClientSize(FVector2D(600.f, 150.f))
		.SupportsMaximize(false).SupportsMinimize(false)
		.SizingRule(ESizingRule::FixedSize)
		[
			SNew(SUDKImportScreen)
		];

	FSlateApplication::Get().AddWindow(UDKImportWindow.ToSharedRef());
}

void FUDKImportPlugin::ShutdownModule()
{
	// Unregister the tab spawner
	FGlobalTabmanager::Get()->UnregisterTabSpawner(UDKImportPluginTabName);
}

#undef LOCTEXT_NAMESPACE