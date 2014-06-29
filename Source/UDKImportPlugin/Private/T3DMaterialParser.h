#pragma once

#include "T3DParser.h"

class T3DLevelParser;

class T3DMaterialParser : public T3DParser
{
public:
	T3DMaterialParser(T3DLevelParser * ParentParser, const FString &Package);
	UMaterial * ImportMaterialT3DFile(const FString &FileName);

private:
	T3DLevelParser * LevelParser;
	
	// T3D Parsing
	UMaterial * ImportMaterial();
	UMaterial * Material;

	UMaterialExpression* ImportMaterialExpression(UClass * Class, FRequirement &TextureRequirement);
	void ImportExpression(FExpressionInput * ExpressionInput);
	void ImportMaterialExpressionFlipBookSample(UMaterialExpressionTextureSample * Expression, FRequirement &TextureRequirement);
	void SetExpression(UObject * Object, FExpressionInput * ExpressionInput);
};
