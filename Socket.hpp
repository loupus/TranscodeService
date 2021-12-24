#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#define DEFAULT_BUFLEN 2048
#define DEFAULT_PORT 27015
#define NI_MAXSERV    32
#define NI_MAXHOST  1025

class cSocket
{
    private:   
    static bool StopFlag;
	pthread_t thHandle;   
    static cSocket* thisObj;
    static std::string GetErrorMessage(int perrcode);
    static void*  ServerThread(void* pParam);
    cSocket()
    {
        
    }
    
    public:
    static cSocket* GetInstance();
    cSocket(cSocket &other) = delete;   
    void operator=(const cSocket &) = delete;
    int Initiliaze();
    int Start();
    int Stop();
    bool SendMsg(std::string pMsg, std::string pDestIp, u_short pPort);
};
