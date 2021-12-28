#pragma once


class BackObject
{
public:
	bool Success = true;
	std::string ErrDesc = "";
	std::string StrValue = "";
	std::wstring GetWStrValue();
};