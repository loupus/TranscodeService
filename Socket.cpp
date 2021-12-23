#include <sstream>
#include "Socket.hpp"

bool cSocket::StopFlag = false;

cSocket *cSocket::thisObj = nullptr;

int cSocket::Initiliaze()
{
    /*
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) 
    {
        printf("WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    if ( iResult != 0 ) {
        printf("getaddrinfo failed with error: %d\n", iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
       // printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    */
    return 0;
}

void *cSocket::ServerThread(void *pParam)
{
    std::cout << "Starting up TCP server" << std::endl;
    int errcode = 0;
    int iRecResult = 0;
    int iSendResult = 0;
    SOCKET server;
    SOCKET client;
    WSADATA wsaData;
    sockaddr_in local;
    sockaddr_in sclient;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    char recvbuf[DEFAULT_BUFLEN] = {0};
    std::stringstream sb;
    std::string GelenMesaj;

    int sclientlen = sizeof(sclient);
    char ClientName[NI_MAXHOST] = {0};
    char ClientService[NI_MAXSERV] = {0};
    char *ClientIp = nullptr;
    int ClientPort = 0;

    int wsaret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (wsaret != 0)
    {
        errcode = WSAGetLastError();
        std::cout << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    local.sin_family = AF_INET;                    //Address family
    local.sin_addr.s_addr = INADDR_ANY;            //Wild card IP address
    local.sin_port = htons((u_short)DEFAULT_PORT); //port to use

    server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == INVALID_SOCKET)
    {
        errcode = WSAGetLastError();
        std::cout << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    if (bind(server, (sockaddr *)&local, sizeof(local)) != 0)
    {
        errcode = WSAGetLastError();
        std::cout << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    if (listen(server, SOMAXCONN) != 0)
    {
        errcode = WSAGetLastError();
        std::cout << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    while (true) //we are looping endlessly
    {
        client = accept(server, (struct sockaddr *)&sclient, &sclientlen);
        if (client == INVALID_SOCKET)
        {
            errcode = WSAGetLastError();
            std::cout << GetErrorMessage(errcode) << std::endl;
            goto exitListen;
        }

        // get client info =========================================================================================
        if (getnameinfo((struct sockaddr *)&sclient, sclientlen, ClientName, NI_MAXHOST, ClientService, NI_MAXSERV, 0) != 0)
        {
            errcode = WSAGetLastError();
            std::cout << "get host name info failed " << GetErrorMessage(errcode) << std::endl;
        }

        ClientIp = inet_ntoa(sclient.sin_addr);
        ClientPort = ntohs(sclient.sin_port);
        // get client info =========================================================================================

        // Receive until the peer shuts down the connection

        sb.str("");
        sb.clear();
        sb.flush();

        do
        {
            memset(recvbuf, 0, DEFAULT_BUFLEN);
            iRecResult = recv(client, recvbuf, DEFAULT_BUFLEN, 0);  // todo gonderirse aliyorsun
            if (iRecResult < 0)
            {
                errcode = WSAGetLastError();
                std::cout << "recv failed: " << GetErrorMessage(errcode) << std::endl;
                goto exitReceive;
            }
            else if (iRecResult > 0)
            {
                sb << recvbuf;
                if(iRecResult < DEFAULT_BUFLEN)         // total mesaj bufferdan kucuk daha bekleme
                break;
                
            }

        } while (iRecResult > 0);

        GelenMesaj = sb.str();
        sb.str("");
        sb.clear();
        sb.flush();

        std::cout << "Connection from " << ClientIp << std::endl;
        std::cout << "Received: " << GelenMesaj << std::endl;

        // send back message ====================================================================================
        memset(sendbuf, 0, DEFAULT_BUFLEN); 
        strcpy(sendbuf,"Hadi len ordan");
        iSendResult = send(client, sendbuf, strlen(sendbuf), 0);
        if (iSendResult == SOCKET_ERROR)
        {
            errcode = WSAGetLastError();
            std::cout << "Send back failed! Client will be disconnected! \n ClientName: " << ClientName << "ClientIP: " << ClientIp << "Err:" << GetErrorMessage(errcode) << std::endl;
            closesocket(client); // bu client ile ilgili kayit alma
        }
       
        // send back message ====================================================================================

    exitReceive:
        closesocket(client);
    }

exitListen:
    closesocket(server);
    WSACleanup();
    std::cout << "Stopping TCP server" << std::endl;
    return nullptr;
}

int cSocket::Start()
{
    int rv = 0;
    StopFlag = false;
    rv = pthread_create(&thHandle, nullptr, &cSocket::ServerThread, (void *)this);
    if (rv != 0)
    {
        //std::cout << dye::red("ERR ===> Transcode thread create failed! HandleX:")  << thHandle.x << std::endl;
        std::cout << "ERR ===> Transcode thread create failed! RV:" << std::endl;
    }
    else
    {
        //std::cout << dye::blue("Thread: ")  << thHandle.x << std::endl;
        //std::cout << hue::blue << "Thread: " << thHandle.x << hue::reset << std::endl;
    }
    return rv;
}

int cSocket::Stop()
{
    int rv = 0;
    void *res;
    StopFlag = true;
    rv = pthread_join(thHandle, &res);
    if (rv != 0)
    {
        std::cout << "	ERR ====> join failed. ErrCode:" << rv << std::endl;
    }

    if (res == PTHREAD_CANCELED)
        std::cout << "thread joined" << std::endl;
    else
    {

        std::cout << "ERR ====> thread join problem! RES:" << (char *)res << std::endl;
    }
    return rv;
}

std::string cSocket::GetErrorMessage(int perrcode)
{
    std::string back;
    char msgbuf[256] = {0};
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, // flags
                   NULL,                                                       // lpsource
                   perrcode,                                                   // message id
                   0,                                                          // languageid
                   msgbuf,                                                     // output buffer
                   sizeof(msgbuf),                                             // size of msgbuf, bytes
                   NULL);                                                      // va_list of arguments

    if (!*msgbuf)
    {
        sprintf(msgbuf, "%d", perrcode); // provide error # if no string available
    }
    back.append(msgbuf);
    LocalFree(msgbuf);
    return back;
}

cSocket *cSocket::GetInstance()
{
    if (thisObj == nullptr)
    {
        thisObj = new cSocket();
    }
    return thisObj;
}

int cSocket::SendMsg(std::string pMsg, std::string pDestIp, u_short pPort)
{
    int iResult;
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in clientService;

    int recvbuflen = DEFAULT_BUFLEN;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    char recvbuf[DEFAULT_BUFLEN] = {0};

    int pStrLen = strlen(pMsg.c_str());
    if (pStrLen < (DEFAULT_BUFLEN - 1))
        strcpy(sendbuf, pMsg.c_str());
    else
        memcpy(sendbuf, pMsg.c_str(), DEFAULT_BUFLEN - 1);

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        wprintf(L"WSAStartup failed with error: %d\n", iResult);
        return 1;
    }

    //----------------------
    // Create a SOCKET for connecting to server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET)
    {
        wprintf(L"socket failed with error: %ld\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(pDestIp.c_str());
    clientService.sin_port = htons(pPort);

    //----------------------
    // Connect to server.
    iResult = connect(ConnectSocket, (SOCKADDR *)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR)
    {
        wprintf(L"connect failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR)
    {
        wprintf(L"send failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        wprintf(L"Bytes received: %d\n", iResult);
        printf("%s\r\n", recvbuf);
    }
    else if (iResult == 0)
        wprintf(L"Connection closed\n");
    else
        wprintf(L"recv failed with error: %d\n", WSAGetLastError());

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR)
    {
        wprintf(L"shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // close the socket
    iResult = closesocket(ConnectSocket);
    if (iResult == SOCKET_ERROR)
    {
        wprintf(L"close failed with error: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    WSACleanup();
    return 0;
}