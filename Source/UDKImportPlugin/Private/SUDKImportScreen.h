#pragma once

DECLARE_LOG_CATEGORY_EXTERN(LogUDKImportPlugin, Warning, All);

class SUDKImportScreen : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(SUDKImportScreen)
	{
	}

	SLATE_END_ARGS()

	/** Widget constructor */
	void Construct(const FArguments& Args);

	/** Run the import tool */
	FReply OnRun();

	/** Import an UDK material from its T3D description*/
	void ImportMaterial(const FString &UdkMaterialT3D);

	/** Run an UDK command and returns the command exitcode */
	int32 SUDKImportScreen::RunUDK(const FString &CommandLine);

	/** Path to the UDK directory (ex: "C:/UDK/UDK-2014-02") */
	TSharedPtr<SEditableText> SUDKPath;

	/** Name of the Map to export */
	TSharedPtr<SEditableText> SLevel;

	/** Path to the temporary directory to use */
	TSharedPtr<SEditableText> STmpPath;


};

