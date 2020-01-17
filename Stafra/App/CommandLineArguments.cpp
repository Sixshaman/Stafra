#include "CommandLineArguments.hpp"
#include <iostream>
#include <regex>
#include <Windows.h>

namespace
{
	const size_t gDefaultPSize     = 10;
	const size_t gDefaultEnlonging = 1;
	const size_t gDefaultSpawn     = 0;

	const bool gDefaultSaveVframes = false;
	const bool gDefaultSmooth      = false;

	//---------------------------------------
	const size_t gMinimumPSize = 2;
	const size_t gMaximumPSize = 14;

	const size_t gMinimumEnlonging = 1;
	const size_t gMaximumEnlonging = 50;

	const size_t gMinimumSpawn = 0;
	const size_t gMaximumSpawn = 9999;
}

CommandLineArguments::CommandLineArguments(const std::string& cmdArgs): mPowSize(gDefaultPSize), mSaveVideoFrames(gDefaultSaveVframes), mSmoothTransform(gDefaultSmooth), mEnlonging(gDefaultEnlonging), mSpawnPeriod(gDefaultSpawn)
{
	auto prevArgEnd = cmdArgs.begin();
	for(auto it = cmdArgs.begin(); it != cmdArgs.end(); ++it)
	{
		if(strchr(" \n\r\t", *it))
		{
			mCmdLineArgs.push_back(std::string(prevArgEnd, it));
			prevArgEnd = it + 1;
		}
	}

	mCmdLineArgs.push_back(std::string(prevArgEnd, cmdArgs.end()));
}

CommandLineArguments::~CommandLineArguments()
{
}

size_t CommandLineArguments::PowSize() const
{
	return mPowSize;
}

size_t CommandLineArguments::Enlonging() const
{
	return mEnlonging;
}

size_t CommandLineArguments::SpawnPeriod() const
{
	return mSpawnPeriod;
}

bool CommandLineArguments::SaveVideoFrames() const
{
	return mSaveVideoFrames;
}

bool CommandLineArguments::SmoothTransform() const
{
	return mSmoothTransform;;
}

size_t CommandLineArguments::ParseInt(std::string intStr, size_t min, size_t max)
{
	size_t parsedNumber = std::strtoull(intStr.c_str(), nullptr, 0);
	if(parsedNumber < min || parsedNumber > max)
	{
		return 0;
	}

	return parsedNumber;
}

CmdParseResult CommandLineArguments::ParseArgs()
{
	CmdParseResult res = CmdParseResult::PARSE_OK;
	for(size_t i = 0; i < mCmdLineArgs.size(); i++)
	{
		if(mCmdLineArgs[i] == "-help")
		{
			res = CmdParseResult::PARSE_HELP;
			break;
		}
		else if(mCmdLineArgs[i] == "-save_vframes")
		{
			mSaveVideoFrames = true;
		}
		else if (mCmdLineArgs[i] == "-smooth")
		{
			mSmoothTransform = true;
		}
		else if(mCmdLineArgs[i] == "-psize")
		{
			if((i + 1) >= mCmdLineArgs.size())
			{
				res = CmdParseResult::PARSE_WRONG_PSIZE;
				break;
			}
			else
			{
				size_t powSize = ParseInt(mCmdLineArgs[++i], gMinimumPSize, gMaximumPSize);
				if(powSize == 0)
				{
					res = CmdParseResult::PARSE_WRONG_PSIZE;
				}
				else
				{
					mPowSize = powSize;
				}
			}
		}
		else if(mCmdLineArgs[i] == "-enlonging")
		{
			if((i + 1) >= mCmdLineArgs.size())
			{
				res = CmdParseResult::PARSE_WRONG_ENLONGING;
				break;
			}
			else
			{
				size_t enlong = ParseInt(mCmdLineArgs[++i], gMinimumEnlonging, gMaximumEnlonging);
				if(enlong == 0)
				{
					res = CmdParseResult::PARSE_WRONG_ENLONGING;
				}
				else
				{
					mEnlonging = enlong;
				}
			}
		}
		else if(mCmdLineArgs[i] == "-spawn")
		{
			if((i + 1) >= mCmdLineArgs.size())
			{
				res = CmdParseResult::PARSE_WRONG_SPAWN;
				break;
			}
			else
			{
				size_t spawn = ParseInt(mCmdLineArgs[++i], gMinimumSpawn, gMaximumSpawn);
				mSpawnPeriod = spawn;
			}
		}
		else
		{
			res = CmdParseResult::PARSE_UNKNOWN_OPTION;
			break;
		}
	}

	return res;
}

std::string CommandLineArguments::GetHelpMessage() const
{
	return "Options:                                                                 \r\n"
		   "                                                                         \r\n"
		   "-save_vframes: Save all intermediate states to the ./DiffStabil folder;  \r\n"
		   "-smooth:       Use smooth transformation for the spawn-stability;        \r\n"
		   "-psize:        The log2 of size of the board. Acceptable range: 2-14;    \r\n"
		   "-enlonging:    Enlonging of the number of steps. Acceptable range: 1-50; \r\n"
		   "-spawn:        Spawn stability period. Enter 0 for no spawn at all.      \r\n";
}

std::string CommandLineArguments::GetErrorMessage(CmdParseResult parseRes) const
{
	switch (parseRes)
	{
	case CmdParseResult::PARSE_OK:
	case CmdParseResult::PARSE_HELP:
		return "";
	case CmdParseResult::PARSE_WRONG_PSIZE:
		return "Wrong pow size entered. Acceptable range: 2-14";
	case CmdParseResult::PARSE_WRONG_ENLONGING:
		return "Wrong enlonging entered. Acceptable range: 1-50";
	case CmdParseResult::PARSE_WRONG_SPAWN:
		return "Wrong spawn period entered";
	case CmdParseResult::PARSE_UNKNOWN_OPTION:
		return "Unknown option. Enter -help to get the list of acceptable options";
	default:
		break;
	}

	return "";
}
