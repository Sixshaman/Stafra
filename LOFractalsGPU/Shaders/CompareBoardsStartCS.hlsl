//Comparison between gBoardLeft and gBoardRight
//Creates a texture of bools from two textures

Texture2D<uint> gBoardLeft:  register(t0);
Texture2D<uint> gBoardRight: register(t1);

RWTexture2D<uint> gOutEquality: register(u0);

[numthreads(32, 32, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	gOutEquality[DTid.xy] = (gBoardLeft[DTid.xy] == gBoardRight[DTid.xy]);
}