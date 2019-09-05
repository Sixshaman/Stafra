#include <iostream>
#include <clocale>
#include <string>
#include <vector>
#include "Util.hpp"
#include "FractalGen.hpp"

int main(int argc, char* argv[])
{
	size_t powSize = 3;
	std::cout << "Enter pow size(2-14): ";
	std::cin  >> powSize;

	char saveVFramesOpt = '\0';
	do
	{
		std::cout << "Save video frames? Y/N? ";
		std::cin  >> saveVFramesOpt;
	} while(!strchr("YyNn", saveVFramesOpt));

	bool saveFrames = (saveVFramesOpt == 'Y' || saveVFramesOpt == 'y');

	size_t enlonging = 1;
	std::cout << "Enter enlonging (1-50): ";
	std::cin >> enlonging;

	uint32_t spawnperiod = 0;
	std::cout << "Enter spawn period (0 to no spawn): ";
	std::cin >> spawnperiod;

	bool useSmooth = false;
	if(spawnperiod != 0)
	{
		char useSmoothTransform = '\0';
		do
		{
			std::cout << "Use smooth transform? Y/N? ";
			std::cin >> useSmoothTransform;
		} while (!strchr("YyNn", useSmoothTransform));

		useSmooth = (useSmoothTransform == 'Y' || useSmoothTransform == 'y');
	}

	if(powSize >= 15 || powSize <= 1 || enlonging >= 50 || enlonging < 1)
	{
		std::cout << "Not supported" << std::endl;
	}
	else
	{
		try
		{
			FractalGen fg(powSize, spawnperiod);
			fg.ComputeFractal(saveFrames, enlonging);
			fg.SaveFractalImage("STABILITY.png", useSmooth);
		}
		catch (DXException ex)
		{
			char locale[256];
			size_t cnt = 0;
			getenv_s(&cnt, locale, "LANG");

			setlocale(LC_ALL, locale);

			if (ex.ToHR() == DXGI_ERROR_DEVICE_REMOVED)
			{
				std::cout << std::endl << std::endl << std::endl;
				std::cout << "DXGI_ERROR_DEVICE_REMOVED detected (probably killed by TDR mechanism). Your graphics card isn't good enough." << std::endl;
				std::cout << std::endl << std::endl << std::endl;
			}
			else
			{
				std::cout << std::endl << std::endl << std::endl;
				std::cout << ex.ToString();
				std::cout << std::endl << std::endl << std::endl;
			}
		}
	}

	system("pause");
	return 0;
}