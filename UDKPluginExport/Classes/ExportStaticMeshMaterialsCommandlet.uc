class ExportStaticMeshMaterialsCommandlet extends Commandlet;

function string FullName(Object O)
{
	local string Url;
	local string ClassName;

	ClassName = string(O.Class);
	Url = string(O.Name);

	O = O.Outer;
	while (O != None)
	{
		Url = O.Name $ "." $ Url;
		O = O.Outer;
	}

	return ClassName $ "'" $ Url$ "'";
}

event int Main( string Params )
{
	local array<string> References;
	local StaticMesh SM;
	local StaticMeshComponent SMC;
	local int i, j;

	ParseStringIntoArray(Params, References, " ", true);

	SMC = new (self) class'StaticMeshComponent';

	for(i = 0; i < References.Length; ++i)
	{
		SM = StaticMesh(DynamicLoadObject(References[i], class'StaticMesh'));
		SMC.SetStaticMesh(SM);
	
		for(j = 0; j < SMC.GetNumElements(); ++j)
		{
			`Log(References[i] @ j @ FullName(SMC.GetMaterial(j)));
		}
	}

	return 0;
}

defaultproperties
{
	LogToConsole=true
}