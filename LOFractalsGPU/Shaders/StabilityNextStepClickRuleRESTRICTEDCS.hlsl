Texture2D<uint> gPrevBoard:     register(t0);
Texture2D<uint> gPrevStability: register(t1);
Texture2D<uint> gClickRule:     register(t2);
Texture2D<uint> gRestriction:   register(t3);

RWTexture2D<uint> gNextBoard:     register(u0);
RWTexture2D<uint> gNextStability: register(u1);

groupshared uint clickRuleDataCache[1024];

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID, uint3 GTid: SV_GroupThreadID, uint GroupIndex: SV_GroupIndex)
{
	clickRuleDataCache[GroupIndex] = gClickRule[GTid.xy];
	GroupMemoryBarrierWithGroupSync();

	uint sum = 0;

	for(uint i = 0; i < 1024; i++)
	{
		int x = i % 32 - 15;
		int y = i / 32 - 15;

		int2 cellPos = int2(DTid.x - x, DTid.y - y);

		uint cellState = gPrevBoard[cellPos] * gRestriction[cellPos];
		sum += clickRuleDataCache[i] * cellState;
	}

	//Using reflectivity and symmetry
	uint thisCellState = gPrevBoard[int2(DTid.x, DTid.y)];
	uint nextCellState = sum % 2;

	uint prevStability = gPrevStability[DTid.xy];
	uint nextStability = (prevStability && (thisCellState == nextCellState));

	gNextBoard[DTid.xy]     = nextCellState;
	gNextStability[DTid.xy] = nextStability * gRestriction[DTid.xy];
}