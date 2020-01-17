Texture2D<float> gInputTex: register(t0);

SamplerState gBestSamplerEver: register(s0);

struct PixelIn
{
	float4 PosH: SV_POSITION;
	float2 TexC: TEXCOORD;
};

float main(PixelIn pin): SV_TARGET
{
	return gInputTex.Sample(gBestSamplerEver, pin.TexC);
}