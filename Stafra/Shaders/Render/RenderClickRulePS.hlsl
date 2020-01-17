Texture2D<uint> gClickRuleTex: register(t0);

struct PixelIn
{
	float4 PosH: SV_POSITION;
	float2 TexC: TEXCOORD;
};

float2 CorrectTexCoords(float2 texC, float2 imageSize)
{
	float2 truncImageSize = imageSize - float2(1.0f, 1.0f); //Bottom and right texels shouldn't be used
	return texC * imageSize / truncImageSize;
}

float4 main(PixelIn pin): SV_TARGET
{
	uint clickRuleWidth  = 0;
	uint clickRuleHeight = 0;
	gClickRuleTex.GetDimensions(clickRuleWidth, clickRuleHeight);

	float2 clickRuleSize = float2(clickRuleWidth, clickRuleHeight);
	uint2 clickRuleCoords = (uint2)(pin.TexC * clickRuleSize);

	float  clickRuleVal   = gClickRuleTex[clickRuleCoords];
	float4 clickRuleColor = clickRuleVal * float4(0.0f, 1.0f, 0.0f, 1.0f);

	float2 texCCorr = CorrectTexCoords(pin.TexC, clickRuleSize);

	//Draw the overlay
	float4 overlayColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	if(abs(texCCorr.x * 2.0f       - 1.0f) < 0.005f  //Horizontal middle
	|| abs(texCCorr.y * 2.0f       - 1.0f) < 0.005f  //Vertical middle
	|| abs(texCCorr.x - texCCorr.y + 0.0f) < 0.001f  //Diagonal top-left to bottom-right
	|| abs(texCCorr.x + texCCorr.y - 1.0f) < 0.001f) //Diagonal top-right to bottom-left
	{
		overlayColor = float4(0.5f, 0.5f, 0.5f, 0.0f);
	}

	return clickRuleColor + overlayColor;
}