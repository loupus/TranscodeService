#include <iomanip>
#include <codecvt>
#include <locale>
#include "consolecolor/color.hpp"
#include "Logger.hpp"

std::ofstream Logger::os;
std::stringstream Logger::ss;
std::string Logger::logfile;
std::string Logger::logtype;
std::mutex Logger::mtx;
std::vector<LogCallbackArg *> Logger::Callbacks;

void Logger::registerCallback(void *pobj, LogCallback pfunction, const std::string &pcallerID)
{
    LogCallbackArg *cba = new LogCallbackArg();
    cba->obj = pobj;
    cba->cb = pfunction;
    cba->ID = pcallerID;
    Callbacks.push_back(cba);
}
void Logger::unRegisterCallback(const std::string &pGuid)
{
    for (std::vector<LogCallbackArg *>::iterator it = Callbacks.begin(); it != Callbacks.end(); ++it)
    {
        if ((*it)->ID == pGuid)
        {
            Callbacks.erase(it);
            it--;
        }
    }
}

void Logger::WriteLog(std::string pLog, LogType pLogType, bool pPersist)
{

    std::unique_lock<std::mutex> lck(mtx);

    time_t now = time(0);
    tm *ltm = localtime(&now);

    logfile.clear();
    logtype.clear();
    ss.clear();
    ss.str("");

    /*
    if (Config::logFolder.empty())
        logfile = ".\\";
    else
        logfile = Config::logFolder;
        */

    //logfile name
    ss << std::put_time(ltm, "%Y-%m-%d");
    logfile.append(ss.str());
    logfile.append(".txt");

    ss << " " << std::put_time(ltm, "%X");

    switch (pLogType)
    {
    case LogType::error:
    {
        logtype = "ERROR";
        break;
    }
    case LogType::warning:
    {
        logtype = "WARNING";
        break;
    }
    case LogType::info:
    {
        logtype = "INFO";
        break;
    }
    case LogType::userevent:
    {
        logtype = "USEREVENT";
        break;
    }
    }

    ss << " : " << logtype.c_str() << " : " << pLog.c_str();

    /*
    std::codecvt_utf8<wchar_t> *ct = new std::codecvt_utf8<wchar_t>();
    std::locale loc(std::locale(), ct);
    os.imbue(loc);
    */

    if (pPersist)
    {
        try
        {
            os.open(logfile.c_str(), std::ios::out | std::ios::app);
        }
        catch (std::ofstream::failure &e)
        {
            std::cerr << "Exception opening/reading/closing file\n";
        }

        try
        {
            os << ss.str() << std::endl;
        }
        catch (const std::exception &)
        {
            std::cout << "cannot persist log string" << std::endl;
        }
    }

    std::cout << ss.str() << std::endl;

    os.close();
    ss.clear();
}

void Logger::FlushLog()
{
    ss.flush();
    os.flush();
}