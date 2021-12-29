#pragma once

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

class BackObject
{
public:
	bool Success = true;
	std::string ErrDesc = "";
	std::string StrValue = "";
	std::wstring GetWStrValue();
};