#pragma once 
#include "AssetTime.hpp"
class cItemInfo
{
public:
    AssetTime ftime;
    std::string clientip = "";
    std::string clienthost = "";
    std::string assetid = "";
    std::string infile = "";
    std::string outfile = "";
    int width = 0;
    int height = 0;
    int vbitratekb = 0;
    int abitratekb = 0;
    std::string vencoder = "";
    std::string aencoder = "";
};

enum ItemState
{
    none = 0,
    queued = 1,
    completed = 2,
    failed = 9
};