#pragma once
#include <ctime>
#include <sstream>
#include <iomanip>


class AssetTime
{
public:
	time_t atime;

	AssetTime() = default;

	// copy constructors
	AssetTime(const AssetTime &other)
	{
		atime = other.atime;
	}

	AssetTime(const time_t &other)
	{
		atime = other;
	}
	//////////////////////////////////////

	// assignment operators;
	AssetTime &operator=(AssetTime other)
	{

		std::swap(atime, other.atime);
		return *this;
	}

	AssetTime &operator=(time_t other)
	{
		atime = other;
		return *this;
	}
	/////////////////////////////////

	std::string asString()
	{
		std::tm *tmpFirstTime = std::localtime(&atime);
		std::ostringstream dtss;
		dtss << std::put_time(tmpFirstTime, "%Y-%m-%d %H:%M:%S");
		return dtss.str();
	}

	void fromString(const char *pStr)
	{
		std::tm t;
		std::istringstream ss(pStr);
		ss >> std::get_time(&t, "%Y-%m-%d %H:%M:%S");
		atime = mktime(&t);
	}

	void fromTString(const char *pStr)
	{
		std::tm t;
		std::istringstream ss(pStr);
		ss >> std::get_time(&t, "%Y-%m-%dT%H:%M:%SZ");
		t.tm_hour += 3;   // anadolu ajansı gmt 0 çalışıyor
		atime = mktime(&t);
	}


	time_t asTimeT()
	{
		return atime;
	}
};