#include "CommandLineArguments.hpp"
#include <iostream>
#include <regex>
#include <Windows.h>

namespace
{
	const uint32_t gDefaultPSize      = 10;
	const uint32_t gDefaultFinalFrame = 0;
	const uint32_t gDefaultSpawn      = 0;

	const bool gDefaultSaveVframes = false;
	const bool gDefaultSmooth      = false;

	//---------------------------------------
	const uint32_t gMinimumPSize = 2;
	const uint32_t gMaximumPSize = 14;

	const uint32_t gMinimumFinalFrame = 1;
	const uint32_t gMaximumFinalFrame = UINT_MAX;

	const uint32_t gMinimumSpawn = 0;
	const uint32_t gMaximumSpawn = 9999;
}

CommandLineArguments::CommandLineArguments(int argc, char* argv[]): CommandLineArguments()
{
	for(int i = 0; i < argc; i++)
	{
		mCmdLineArgs.push_back(std::string(argv[i]));
	}
}

CommandLineArguments::CommandLineArguments(const std::string& cmdArgs): CommandLineArguments()
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

CommandLineArguments::CommandLineArguments(): mPowSize(gDefaultPSize), mSaveVideoFrames(gDefaultSaveVframes), mSmoothTransform(gDefaultSmooth), 
                                              mFinalFrame(gDefaultFinalFrame), mSpawnPeriod(gDefaultSpawn), 
	                                          mSilentMode(false), mResetMode(CmdResetMode::RESET_4_CORNERS), mHelpOnly(false)
{
}

CommandLineArguments::~CommandLineArguments()
{
}

uint32_t CommandLineArguments::PowSize() const
{
	return mPowSize;
}

uint32_t CommandLineArguments::FinalFrame() const
{
	return mFinalFrame;
}

uint32_t CommandLineArguments::SpawnPeriod() const
{
	return mSpawnPeriod;
}

bool CommandLineArguments::HelpOnly() const
{
	return mHelpOnly;
}

bool CommandLineArguments::SaveVideoFrames() const
{
	return mSaveVideoFrames;
}

bool CommandLineArguments::SmoothTransform() const
{
	return mSmoothTransform;
}

bool CommandLineArguments::SilentMode() const
{
	return mSilentMode;
}

CmdResetMode CommandLineArguments::ResetMode() const
{
	return mResetMode;
}

uint32_t CommandLineArguments::ParseInt(std::string intStr, uint32_t min, uint32_t max)
{
	uint32_t parsedNumber = std::strtoul(intStr.c_str(), nullptr, 0);
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
			res       = CmdParseResult::PARSE_HELP;
			mHelpOnly = true;
		}
		else if(mCmdLineArgs[i] == "-save_vframes")
		{
			mSaveVideoFrames = true;
		}
		else if (mCmdLineArgs[i] == "-smooth")
		{
			mSmoothTransform = true;
		}
		else if (mCmdLineArgs[i] == "-silent")
		{
			mSilentMode = true;
		}
		else if(mCmdLineArgs[i] == "-reset_mode")
		{
			if((i + 1) >= mCmdLineArgs.size())
			{
				res = CmdParseResult::PARSE_WRONG_RESET_MODE;
				break;
			}
			else
			{
				std::string resetModeStr = mCmdLineArgs[++i];
				if(resetModeStr == "4corners")
				{
					mResetMode = CmdResetMode::RESET_4_CORNERS;
				}
				else if(resetModeStr == "4sides")
				{
					mResetMode = CmdResetMode::RESET_4_SIDES;
				}
				else if(resetModeStr == "center")
				{
					mResetMode = CmdResetMode::RESET_CENTER;
				}
				else
				{
					res = CmdParseResult::PARSE_WRONG_RESET_MODE;
					break;
				}
			}
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
				uint32_t powSize = ParseInt(mCmdLineArgs[++i], gMinimumPSize, gMaximumPSize);
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
		else if(mCmdLineArgs[i] == "-final_frame")
		{
			if((i + 1) >= mCmdLineArgs.size())
			{
				res = CmdParseResult::PARSE_WRONG_FINAL_FRAME;
				break;
			}
			else
			{
				uint32_t finalFrame = ParseInt(mCmdLineArgs[++i], gMinimumFinalFrame, gMaximumFinalFrame);
				if(finalFrame == 0)
				{
					res = CmdParseResult::PARSE_WRONG_FINAL_FRAME;
				}
				else
				{
					mFinalFrame = finalFrame;
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
				uint32_t spawn = ParseInt(mCmdLineArgs[++i], gMinimumSpawn, gMaximumSpawn);
				mSpawnPeriod = spawn;
			}
		}
		else
		{
			//It's fine, don't finish
			//res = CmdParseResult::PARSE_UNKNOWN_OPTION;
			//break;
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
		   "-final_frame:  The frame number that will be saved.                      \r\n"
		   "-spawn:        Spawn stability period. Enter 0 for no spawn at all.      \r\n"
		   "-reset_mode:   Reset mode. Available values: 4corners | 4sides | center. \r\n";
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
	case CmdParseResult::PARSE_WRONG_FINAL_FRAME:
		return "Wrong final frame entered. Enter the number greater than zero.";
	case CmdParseResult::PARSE_WRONG_SPAWN:
		return "Wrong spawn period entered";
	case CmdParseResult::PARSE_UNKNOWN_OPTION:
		return "Unknown option. Enter -help to get the list of acceptable options";
	default:
		break;
	}

	return "";
}
