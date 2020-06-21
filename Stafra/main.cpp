#include "App/WindowApp.hpp"
#include "App/ConsoleApp.hpp"
#include "App/CommandLineArguments.hpp"

#include <iostream>

int main(int argc, char* argv[])
{
	CommandLineArguments cmdArgs(argc, argv);
	cmdArgs.ParseArgs();

	if(cmdArgs.SilentMode())
	{
		if(cmdArgs.HelpOnly())
		{
			std::cout << cmdArgs.GetHelpMessage() << std::endl;
		}
		else
		{
			ConsoleApp app(cmdArgs);
			if (!cmdArgs.HelpOnly())
			{
				app.ComputeFractal();
			}
		}

		return 0;
	}
	else
	{
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);

		FreeConsole();
		
		if(cmdArgs.HelpOnly())
		{
			MessageBoxA(nullptr, cmdArgs.GetHelpMessage().c_str(), "Help", MB_OK);
			return 0;
		}
		else
		{
			WindowApp app((HINSTANCE)GetModuleHandle(nullptr), cmdArgs);
			return app.Run();
		}
	}
}