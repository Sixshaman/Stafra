struct VertexOut
{
	float4 PosH: SV_POSITION;
	float2 TexC: TEXCOORD;
};

static const float2 screenVertices[] =
{
	float2(-1.0f,  1.0f),
	float2( 1.0f,  1.0f),
	float2(-1.0f, -1.0f),
	float2( 1.0f, -1.0f)
};

static const float2 texcoord[] =
{
	float2(0.0f, 0.0f),
	float2(1.0f, 0.0f),
	float2(0.0f, 1.0f),
	float2(1.0f, 1.0f)
};

VertexOut main(uint vid: SV_VertexID)
{
	VertexOut vout;
	vout.PosH = float4(screenVertices[vid], 0.0f, 1.0f);
	vout.TexC = texcoord[vid];

	return vout;
}