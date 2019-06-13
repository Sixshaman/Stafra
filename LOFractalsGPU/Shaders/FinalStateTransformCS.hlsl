//Transfroms the initial state texture into the stability buffer data

Texture2D<uint> gFinalBoard: register(t0);

RWTexture2D<float> gOutTex: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	gOutTex[DTid.xy] = (float)gFinalBoard[DTid.xy];
}