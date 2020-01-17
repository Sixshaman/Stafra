cbuffer cbSpawnParams
{
	uint gSpawnPeriod;
};

Texture2D<uint> gPrevBoard:     register(t0);
Texture2D<uint> gPrevStability: register(t1);

StructuredBuffer<int2> gClickRuleCoordsBuf: register(t2); //Non-zero cells of the click rule
Buffer<uint>    gClickRuleCoordsCounterBuf: register(t3); //A number of non-zero cells in the click rule

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
		sum += gPrevBoard[int2(DTid.xy) - clickRuleCoord];
	}

	//Using reflectivity and symmetry
	uint thisCellState = gPrevBoard[int2(DTid.x, DTid.y)];
	uint nextCellState = sum % 2;

	uint prevStability = gPrevStability[DTid.xy];
	uint nextStability = prevStability;
	
	[flatten]
	if(thisCellState == nextCellState)
	{
		[flatten]
		if(prevStability != 1)
		{
			nextStability = (prevStability + 1) % (2 + gSpawnPeriod);
		}
	}
	else
	{
		nextStability = 2;
	}

	gNextBoard[DTid.xy]     = nextCellState;
	gNextStability[DTid.xy] = nextStability;
}