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
	
	if(powSize >= 15 || powSize <= 1)
	{
		std::cout << "Not supported" << std::endl;
	}
	else
	{
		try
		{
			FractalGen fg(powSize);
			fg.ComputeFractal();
			fg.SaveFractalImage("STABILITY.png");
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