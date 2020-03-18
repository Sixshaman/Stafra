cbuffer cbParams: register(b0)
{
	uint2 gBoardSize: packoffset(c0);
}

RWTexture2D<uint> gInitialBoard: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	uint2 center = uint2(gBoardSize.x / 2, gBoardSize.y / 2);

	bool res = (DTid.x == center.x && DTid.y == center.y);
	gInitialBoard[DTid.xy] = res;
}