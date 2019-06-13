//Tile shrinkage
//Creates a texture of bools heavily shrinked
//Tile 32x32 -> A bool of "Is the whole tile of gBoard is true"

cbuffer cbParams: register(b0)
{
	uint2 gBoardSize: packoffset(c0);
}

Texture2D<uint> gBoard: register(t0);

RWTexture2D<uint> gOutEquality: register(u0);

groupshared bool equalityData[1024];

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID, uint3 TDid: SV_GroupID, uint GroupIndex: SV_GroupIndex)
{
	if(DTid.x >= gBoardSize.x || DTid.y >= gBoardSize.y)
	{
		equalityData[GroupIndex] = true;
	}
	else
	{
		equalityData[GroupIndex] = gBoard[DTid.xy];
	}

	GroupMemoryBarrierWithGroupSync();

	[unroll]
	for(uint i = 0; i <= 3; i++)
	{
		uint closeIndex  =   1 << (i * 2); //i = [0, 1, 2, 3] -> closeIndex  = [1, 4, 16, 64]
		uint sparseIndex = closeIndex * 4; //i = [0, 1, 2, 3] -> sparseIndex = [4, 16, 64, 256]

		if(GroupIndex % sparseIndex == 0)
		{
			bool eqThis  = equalityData[GroupIndex                 ];
			bool eqNext1 = equalityData[GroupIndex + closeIndex    ];
			bool eqNext2 = equalityData[GroupIndex + closeIndex * 2];
			bool eqNext3 = equalityData[GroupIndex + closeIndex * 3];

			equalityData[GroupIndex] = (eqThis && eqNext1 && eqNext2 && eqNext3);
		}

		GroupMemoryBarrierWithGroupSync();
	}

	if(GroupIndex == 0)
	{
		bool eqThisFinal  = equalityData[0];
		bool eqNext1Final = equalityData[256];
		bool eqNext2Final = equalityData[512];
		bool eqNext3Final = equalityData[768];
		
		bool finalEq = (eqThisFinal && eqNext1Final && eqNext2Final && eqNext3Final);
		gOutEquality[TDid.xy] = finalEq;
	}
}