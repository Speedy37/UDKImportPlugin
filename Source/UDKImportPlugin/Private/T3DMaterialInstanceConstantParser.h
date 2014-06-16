#pragma once

#include "T3DParser.h"

class T3DLevelParser;

class T3DMaterialInstanceConstantParser : public T3DParser
{
public:
	T3DMaterialInstanceConstantParser(T3DLevelParser * ParentParser, const FString &Package);
	UMaterialInstanceConstant * ImportT3DFile(const FString &FileName);

private:
	T3DLevelParser * LevelParser;

	// T3D Parsing
	UMaterialInstanceConstant * ImportMaterialInstanceConstant();
	UMaterialInstanceConstant * MaterialInstanceConstant;
	bool IsParameter(const FString &Key, int32 &index, FString &Value);
};