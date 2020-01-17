//Transfroms the stability buffer data into a colorful image, with smooth transformation

cbuffer cbSpawnParams
{
	uint gSpawnPeriod;
};

Texture2D<uint> gFinalBoard: register(t0);

RWTexture2D<float> gOutTex: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	uint  finalStability = gFinalBoard[DTid.xy];
	float finalVal       = 0.0f;

	[flatten]
	if(gSpawnPeriod == 0) //Smooth transformation is impossible if there is no spawn
	{
		finalVal = (float)finalStability;
	}
	else
	{
		//1 -> gSpawnPeriod
		//2 -> 0 
		//3 -> 1
		//4 -> 2
		//...
		finalStability = (finalStability + gSpawnPeriod - 1) % (gSpawnPeriod + 1);
		finalVal       = (float)finalStability / (float)gSpawnPeriod;
	}

	gOutTex[DTid.xy] = finalVal;
}