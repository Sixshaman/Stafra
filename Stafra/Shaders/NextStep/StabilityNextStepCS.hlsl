Texture2D<uint> gPrevBoard:     register(t0);
Texture2D<uint> gPrevStability: register(t1);

RWTexture2D<uint> gNextBoard:     register(u0);
RWTexture2D<uint> gNextStability: register(u1);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	//Using reflectivity and symmetry
	uint thisCellState    = gPrevBoard[int2(DTid.x,     DTid.y    )];
	uint leftCellState    = gPrevBoard[int2(DTid.x - 1, DTid.y    )];
	uint rightCellState   = gPrevBoard[int2(DTid.x + 1, DTid.y    )];
	uint topCellState     = gPrevBoard[int2(DTid.x,     DTid.y - 1)];
	uint bottomCellState  = gPrevBoard[int2(DTid.x,     DTid.y + 1)];

	uint nextCellState = (thisCellState + leftCellState + rightCellState + topCellState + bottomCellState) % 2;

	uint prevStability = gPrevStability[DTid.xy];
	uint nextStability = (prevStability && (thisCellState == nextCellState));

	gNextBoard[DTid.xy]     = nextCellState;
	gNextStability[DTid.xy] = nextStability;
}