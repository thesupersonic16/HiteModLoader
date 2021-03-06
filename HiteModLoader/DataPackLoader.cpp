#include <pch.h>
#include "Loader.h"
#include "ModLoader.h"
#include "SigScan.h"
#include "Helpers.h"

std::string DataPackName = "";
bool ForceScripts = false;

void GetRedirectedPath(const char* filePath, char* out)
{
	if (DataPackName.empty())
		return;

	for (auto& value : ModIncludePaths)
	{
		std::string path = value + DataPackName + "/" + filePath;
		if (GetFileAttributesA(path.c_str()) != -1)
		{
			strcpy(out, path.c_str());
			return;
		}
	}
}

const char* GetDataPackName()
{
	return DataPackName.c_str();
}

HOOK(FILE*, __fastcall, Engine_LoadFile, SigEngine_LoadFile(), FileInfo* info, const char* filePath, int openMode)
{
	std::string filePath2 = filePath;

	if (filePath2.find("retro/") != std::string::npos)
	{
		std::string newDataPack = filePath2.substr(6, filePath2.length() - 11);
		if (DataPackName != newDataPack)
		{
			DataPackName = newDataPack;
			LOG("[DataPackLoader] Opened Datapack: %s", DataPackName.c_str());
		}
		// Check if scripts should be forced
		ForceScripts = false;
		for (auto& value : ModIncludePaths)
		{
			std::string path = value + DataPackName + "/Data/Scripts/";
			if (GetFileAttributesA(path.c_str()) != -1)
			{
				ForceScripts = true;
				break;
			}
		}
	}

	if (DataPackName.empty())
		return originalEngine_LoadFile(info, filePath, openMode);

	// Force scripts if enabled
	if (ForceScripts && filePath2.find("GlobalCode.bin") != std::string::npos)
		return 0;

	for (auto& value : ModIncludePaths)
	{
		std::string path = value + DataPackName + "/" + filePath;
		if (GetFileAttributesA(path.c_str()) != -1)
		{
			LOG("[DataPackLoader] Loading File: %s", path.c_str());
			info->externalFile = 1;
			return originalEngine_LoadFile(info, path.c_str(), openMode);
		}
	}

	return originalEngine_LoadFile(info, filePath, openMode);
}

HOOK(int, __fastcall, Engine_CloseFile, SigEngine_CloseFile(), FileInfo* info)
{
	info->externalFile = false;
	return originalEngine_CloseFile(info);
}

HOOK(int, __fastcall, Engine_CloseFile2, SigEngine_CloseFile2(), FileInfo* info)
{
	int result = originalEngine_CloseFile2(info);
	memset(info, 0, sizeof(FileInfo));
	return result;
}

void InitDataPackLoader()
{
	LOG("Setting up DataPackLoader...");
	INSTALL_HOOK(Engine_LoadFile);
	INSTALL_HOOK(Engine_CloseFile);
	INSTALL_HOOK(Engine_CloseFile2);

	ModLoaderData.GetRedirectedPath = GetRedirectedPath;
	ModLoaderData.GetDataPackName = GetDataPackName;
}