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

	float2 texCCorr  = CorrectTexCoords(pin.TexC, clickRuleSize);
	float cellWidth  = 1.0f / clickRuleWidth;
	float cellHeight = 1.0f / clickRuleHeight;

	//float leftCellCoordX = texCor

	bool onHorizontalMiddle = abs(texCCorr.x * 2.0f - 1.0f) < 0.005f;
	bool onVerticalMiddle   = abs(texCCorr.y * 2.0f - 1.0f) < 0.005f;
	bool onDiagonalTLBR     = abs(texCCorr.x - texCCorr.y + 0.0f) < 0.001f;
	bool onDiagonalTRBL     = abs(texCCorr.x + texCCorr.y - 1.0f) < 0.001f;

	//Draw the overlay
	float4 overlayColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	if(onHorizontalMiddle || onVerticalMiddle || onDiagonalTLBR || onDiagonalTRBL)
	{
		overlayColor = float4(0.5f, 0.5f, 0.5f, 0.0f);
	}

	return clickRuleColor + overlayColor;
}