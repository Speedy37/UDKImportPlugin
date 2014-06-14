#pragma once

#include "T3DParser.h"

class T3DLevelParser;

class T3DMaterialParser : public T3DParser
{
public:
	T3DMaterialParser(T3DLevelParser * ParentParser);
	void ImportMaterialT3DFile(const FString &FileName, const FString &Package);

private:
	T3DLevelParser * LevelParser;

	// T3D Parsing
	void ImportMaterial(const FString &Package);
	UMaterial * Material;

	UMaterialExpression* ImportMaterialExpression(UClass * Class);
	UMaterialExpression* BeginMaterialExpression(UClass * Class);
	void EndMaterialExpression();
	void ImportExpression(FExpressionInput * ExpressionInput);
	void SetExpression(UObject * Object, FExpressionInput * ExpressionInput);
};
