#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include "nlohmann/json.hpp"
#include "ItemInfo.hpp"

#define DEFAULT_BUFLEN 2048
#define DEFAULT_PORT 27015
#define NI_MAXSERV 32
#define NI_MAXHOST 1025



class cServerResponse
{
private:
  std::string getitemstatestr()
  {
      switch(iitemstate)
      {
          case ItemState::none:
          return "none";
          case ItemState::queued:
          return "queued";
          case ItemState::completed:
          return "completed";
          case ItemState::failed:
          return "failed";
      }
      return "";
  }

public:
    std::string assetid="";
    bool success=false;
    std::string itemstate = "none";
    int iitemstate = ItemState::none;
    std::string errmessage="";
    std::string exmessage="";


    std::string toJsonStr()
    {
        nlohmann::json j = nlohmann::json
        {
            {"assetid", this->assetid},
            {"success", this->success},
            {"itemstate", this->getitemstatestr()},
            {"iitemstate", this->iitemstate},
            {"errmessage", this->errmessage},
            {"exmessage", this->exmessage}
        };
        return j.dump();
    }
};

class cSocket
{
private:
    static bool StopFlag;
    pthread_t thHandle;
    static cSocket *thisObj;
    static std::string GetErrorMessage(int perrcode);
    static void *ServerThread(void *pParam);
    static cItemInfo EvalMessage(std::string pMsg);
    cSocket()
    {
    }

public:
    static cSocket *GetInstance();
    cSocket(cSocket &other) = delete;
    void operator=(const cSocket &) = delete;
    int Initiliaze();
    int Start();
    int Stop();
    bool SendMsg(std::string pMsg, std::string pDestIp, u_short pPort);
};
