#pragma once
#include <iostream>
#include <vector>
#include <mutex>
#include <sstream>  
#include <fstream>

typedef void(*LogCallback)(void*, std::string&);

struct LogCallbackArg
{
    LogCallback cb;
    void* obj;
    std::string ID;
};

enum class LogType
{
    info,
    error,
    warning,
    userevent
};

class Logger
{

    static std::mutex mtx;
    static std::vector<LogCallbackArg*> Callbacks;
    static std::stringstream ss;
    static std::ofstream  os;
    static std::string logfile;
    static std::string logtype;

public:

    static void registerCallback(void* pobj, LogCallback pfunction, const std::string& pcallerID);
    static void unRegisterCallback(const std::string& pGuid);
    static void WriteLog(std::string pLog, LogType pLogType = LogType::info, bool pPersist=true);

    template <typename ...Args>
    static void WriteFLog(std::string pLog, LogType pLogType, bool pPersist, char* sFormat, Args... args);
    static void FlushLog();
    Logger(Logger const&) = delete;
    void operator=(Logger const&) = delete;

};