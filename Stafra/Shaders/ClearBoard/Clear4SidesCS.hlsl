cbuffer cbParams: register(b0)
{
	uint2 gBoardSize: packoffset(c0);
}

RWTexture2D<uint> gInitialBoard: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	uint2 left   = uint2(0,                gBoardSize.y / 2);
	uint2 right  = uint2(gBoardSize.x - 1, gBoardSize.y / 2);
	uint2 top    = uint2(gBoardSize.x / 2, 0               );
	uint2 bottom = uint2(gBoardSize.x / 2, gBoardSize.y - 1);

	bool res = all(DTid.xy == left) || all(DTid.xy == right) || all(DTid.xy == top) || all(DTid.xy == bottom);
	gInitialBoard[DTid.xy] = res;
}