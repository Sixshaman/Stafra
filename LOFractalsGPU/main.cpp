#include <iostream>

#include <string>
#include <vector>
#include "Util.hpp"
#include "FractalGen.hpp"
#include "CommandLineArguments.hpp"

int main(int argc, char* argv[])
{
	CommandLineArguments cmdArgs(argc, argv);

	CmdParseResult parseRes = cmdArgs.ParseArgs();
	if(parseRes == CmdParseResult::PARSE_HELP)
	{
		std::cout << cmdArgs.GetHelpMessage();
		return 0;
	}
	else if(parseRes != CmdParseResult::PARSE_OK)
	{
		std::cout << cmdArgs.GetErrorMessage(parseRes);
		return 1;
	}

	try
	{
		FractalGen fg(cmdArgs.PowSize(), cmdArgs.SpawnPeriod());
		fg.ComputeFractal(cmdArgs.SaveVideoFrames(), cmdArgs.SmoothTransform(), cmdArgs.Enlonging());
		fg.SaveFractalImage("STABILITY.png", cmdArgs.SmoothTransform());
	}
	catch (DXException ex)
	{
		std::cout << ex.ToString();
	}

	return 0;
}