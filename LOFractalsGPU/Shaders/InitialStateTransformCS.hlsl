//Transfroms the initial state texture into the stability buffer data

Texture2D<float4> gInitialStateTex: register(t0);

RWTexture2D<uint> gInitialBoard: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	float3 lumCoeff   = float3(0.2126f, 0.7152f, 0.0722f);
	float3 stateColor = gInitialStateTex[DTid.xy].xyz;

	gInitialBoard[DTid.xy] = (dot(lumCoeff, stateColor) > 0.15f);
}