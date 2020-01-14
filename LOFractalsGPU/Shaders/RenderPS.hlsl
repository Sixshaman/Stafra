Texture2D<float> gInputTex: register(t0);

SamplerState gBoardSampler: register(s0);

struct PixelIn
{
	float4 PosH: SV_POSITION;
	float2 TexC: TEXCOORD;
};

float4 main(PixelIn pin): SV_TARGET
{
	float  boardVal       = gInputTex.Sample(gBoardSampler, pin.TexC);
	float4 stabilityColor = float4(1.0f, 0.0f, 1.0f, 1.0f);

	return stabilityColor * boardVal;
}