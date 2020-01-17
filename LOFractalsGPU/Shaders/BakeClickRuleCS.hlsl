//Writes non-zero element coordinates from the input texture into the buffer

Texture2D<uint>              gClickRuleTex:      register(t0);
AppendStructuredBuffer<int2> gClickRuleCoordBuf: register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid: SV_DispatchThreadID)
{
	uint val = gClickRuleTex[DTid.xy];
	if(val > 0)
	{
		uint clickRuleWidth;
		uint clickRuleHeight;
		gClickRuleTex.GetDimensions(clickRuleWidth, clickRuleHeight);

		int2 clickOffset = (int2)(DTid.xy) - int2(clickRuleWidth / 2, clickRuleHeight / 2);
		gClickRuleCoordBuf.Append(clickOffset);
	}
}