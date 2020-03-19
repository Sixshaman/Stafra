cbuffer cbParams: register(b0)
{
	uint2 gBoardSize: packoffset(c0);
}

RWTexture2D<uint> gInitialBoard: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	uint2 topLeft     = uint2(0,                               0);
	uint2 topRight    = uint2(gBoardSize.x - 1,                0);
	uint2 bottomLeft  = uint2(0,                gBoardSize.y - 1);
	uint2 bottomRight = uint2(gBoardSize.x - 1, gBoardSize.y - 1);

	bool res = all(DTid.xy == topLeft) || all(DTid.xy == topRight) || all(DTid.xy == bottomLeft) || all(DTid.xy == bottomRight);
	gInitialBoard[DTid.xy] = res;
}