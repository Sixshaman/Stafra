Texture2D<uint> gInput: register(t0);

RWTexture2D<uint> gOutput: register(u0);

[numthreads(16, 16, 1)]
void main(uint3 DTid: SV_DispatchThreadID )
{
	uint inputWidth  = 0;
	uint inputHeight = 0;

	uint outputWidth  = 0;
	uint outputHeight = 0;

	gInput.GetDimensions(inputWidth, inputHeight);
	gOutput.GetDimensions(outputWidth, outputHeight);

	int2 newInputLeftTop     = (((int)outputWidth - (int)inputWidth) / 2, ((int)outputHeight - (int)inputHeight) / 2);
	int2 newInputRightBottom = (newInputLeftTop.x + inputWidth, newInputLeftTop.y + inputHeight);
	
	if(any((int2)DTid.xy < newInputLeftTop))
	{
		gOutput[DTid.xy] = 0;
	}
	else if(any((int2)DTid.xy > newInputRightBottom))
	{
		gOutput[DTid.xy] = 0;
	}
	else
	{
		gOutput[DTid.xy] = gInput[(int2)DTid.xy - newInputLeftTop];
	}
}