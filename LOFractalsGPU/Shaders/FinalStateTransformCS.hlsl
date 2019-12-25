//Transfroms the initial state texture into the stability buffer data

Texture2D<uint> gFinalBoard: register(t0);

RWTexture2D<float> gOutTex: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	uint finalStability = gFinalBoard[DTid.xy];
	
	[flatten]
	if(finalStability >= 2)
	{
		finalStability = 0;
	}

	gOutTex[DTid.xy] = (float)finalStability;
}