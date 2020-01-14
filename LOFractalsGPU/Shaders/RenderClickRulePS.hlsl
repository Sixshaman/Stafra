Texture2D<uint> gClickRuleTex: register(t0);

SamplerState gBoardSampler: register(s0);

struct PixelIn
{
	float4 PosH: SV_POSITION;
	float2 TexC: TEXCOORD;
};

float4 main(PixelIn pin): SV_TARGET
{
	uint clickRuleWidth  = 0;
	uint clickRuleHeight = 0;
	gClickRuleTex.GetDimensions(clickRuleWidth, clickRuleHeight);

	//Bottom-right part of click rule is not used
	clickRuleWidth  = clickRuleWidth  - 1;
	clickRuleHeight = clickRuleHeight - 1;

	uint2 clickRuleCoords = (uint2)(pin.TexC * float2(clickRuleWidth, clickRuleHeight));

	float  clickRuleVal   = gClickRuleTex[clickRuleCoords];
	float4 clickRuleColor = float4(0.0f, 1.0f, 0.0f, 1.0f);

	return clickRuleColor * clickRuleVal;
}