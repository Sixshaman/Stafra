#define DRAW_OVERLAY 0x01

cbuffer cbParams: register(b0)
{
	uint gDrawFlags;
};

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

	float4 overlayColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
	if(gDrawFlags & DRAW_OVERLAY)
	{
		float2 texCCorr  = CorrectTexCoords(pin.TexC, clickRuleSize);
		float cellWidth  = 1.0f / clickRuleWidth;
		float cellHeight = 1.0f / clickRuleHeight;

		float leftCellCoordX   = texCCorr.x -                    0  * cellWidth;
		float rightCellCoordX  = texCCorr.x - (clickRuleSize.x - 1) * cellWidth;
		float topCellCoordY    = texCCorr.y -                    0  * cellHeight;
		float bottomCellCoordY = texCCorr.y - (clickRuleSize.y - 1) * cellHeight;

		bool onHorizontalMiddle = abs(texCCorr.x * 2.0f - 1.0f) < 0.005f;
		bool onVerticalMiddle   = abs(texCCorr.y * 2.0f - 1.0f) < 0.005f;
		bool onDiagonalTLBR     = abs(texCCorr.x - texCCorr.y + 0.0f) < 0.001f;
		bool onDiagonalTRBL     = abs(texCCorr.x + texCCorr.y - 1.0f) < 0.001f;

		bool onLeftCellCenter   = abs(leftCellCoordX   - 0.5f * cellWidth)  < 0.001f;
		bool onRightCellCenter  = abs(rightCellCoordX  - 0.5f * cellWidth)  < 0.001f;
		bool onTopCellCenter    = abs(topCellCoordY    - 0.5f * cellHeight) < 0.001f;
		bool onBottomCellCenter = abs(bottomCellCoordY - 0.5f * cellHeight) < 0.001f;

		//Draw the overlay
		if(onHorizontalMiddle || onVerticalMiddle  || onDiagonalTLBR  || onDiagonalTRBL 
		|| onLeftCellCenter   || onRightCellCenter || onTopCellCenter || onBottomCellCenter)
		{
			overlayColor = float4(0.5f, 0.5f, 0.5f, 0.0f);
		}
	}

	return clickRuleColor + overlayColor;
}