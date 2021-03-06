Texture2D<uint> gPrevBoard:     register(t0);
Texture2D<uint> gPrevStability: register(t1);
Texture2D<uint> gRestriction:   register(t2);

StructuredBuffer<int2> gClickRuleCoordsBuf: register(t3); //Non-zero cells of the click rule
Buffer<uint>    gClickRuleCoordsCounterBuf: register(t4); //A number of non-zero cells in the click rule

RWTexture2D<uint> gNextBoard:     register(u0);
RWTexture2D<uint> gNextStability: register(u1);

groupshared int2 clickRuleDataCache[1024];
groupshared uint clickRuleCounterVal;

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID, uint3 GTid: SV_GroupThreadID, uint GroupIndex: SV_GroupIndex)
{
	if(GroupIndex == 0) //Copy the counter value
	{
		clickRuleCounterVal = gClickRuleCoordsCounterBuf[0];
	}

	GroupMemoryBarrierWithGroupSync();

	if(GroupIndex < clickRuleCounterVal) //Copy the click rule cells
	{
		clickRuleDataCache[GroupIndex] = gClickRuleCoordsBuf[GroupIndex];
	}
	else
	{
		clickRuleDataCache[GroupIndex] = int2(0, 0);
	}

	GroupMemoryBarrierWithGroupSync();

	uint sum = 0;
	for(uint i = 0; i < clickRuleCounterVal; i++)
	{
		int2 clickRuleCoord = clickRuleDataCache[i];
		int2 cellCoord      = int2(DTid.xy) - clickRuleCoord;

		sum += gPrevBoard[cellCoord] * gRestriction[cellCoord];
	}

	//Using reflectivity and symmetry
	uint thisCellState = gPrevBoard[int2(DTid.x, DTid.y)];
	uint nextCellState = sum % 2;

	uint prevStability = gPrevStability[DTid.xy];
	uint nextStability = (prevStability && (thisCellState == nextCellState));

	gNextBoard[DTid.xy]     = nextCellState;
	gNextStability[DTid.xy] = nextStability * gRestriction[DTid.xy];
}