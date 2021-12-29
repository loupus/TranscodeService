#include <sstream>
#include "ServerResponse.hpp"
#include "Socket.hpp"

// todo concurent thread safe cout

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
    if(pParam == nullptr)
    {
        std::cout << "Server failed to start: null argument" << std::endl;
        return nullptr;
    }
    cSocket* thisObj = reinterpret_cast<cSocket*>(pParam);
    std::cout << "Starting up TCP server" << std::endl;
    int iResult = 0;
    int errcode = 0;
    SOCKET server;
    SOCKET client;
    WSADATA wsaData;
    sockaddr_in local;
    sockaddr_in sclient;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    char recvbuf[DEFAULT_BUFLEN] = {0};
    int SendReceiveTimeout = 3000;
    std::stringstream sb;
    std::string GelenMesaj;
    std::string GidenMesaj;

    int sclientlen = sizeof(sclient);
    char ClientName[NI_MAXHOST] = {0};
    char ClientService[NI_MAXSERV] = {0};
    char *ClientIp = nullptr;
    // int ClientPort = 0;
    cItemInfo temp;
    cServerResponse sr;

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

    iResult = bind(server, (sockaddr *)&local, sizeof(local));
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    iResult = setsockopt(server, SOL_SOCKET, SO_RCVTIMEO, (char *)&SendReceiveTimeout, sizeof(int));
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: set receive timeout failed: " << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    iResult = setsockopt(server, SOL_SOCKET, SO_SNDTIMEO, (char *)&SendReceiveTimeout, sizeof(int));
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: set send timeout failed: " << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    iResult = listen(server, SOMAXCONN);
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << GetErrorMessage(errcode) << std::endl;
        goto exitListen;
    }

    while (true) //we are looping endlessly
    {
        client = accept(server, (struct sockaddr *)&sclient, &sclientlen); // accept is blocking other queue members
        if (client == INVALID_SOCKET)
        {
            errcode = WSAGetLastError();
            std::cout << GetErrorMessage(errcode) << std::endl;
            goto exitListen;
        }

        // get client info =========================================================================================
        iResult = getnameinfo((struct sockaddr *)&sclient, sclientlen, ClientName, NI_MAXHOST, ClientService, NI_MAXSERV, 0);
        if (iResult == SOCKET_ERROR)
        {
            errcode = WSAGetLastError();
            std::cout << "SERVER: Get host name info failed " << GetErrorMessage(errcode) << std::endl;
        }

        ClientIp = inet_ntoa(sclient.sin_addr);
        //ClientPort = ntohs(sclient.sin_port);
        // get client info =========================================================================================

        // Receive until the peer shuts down the connection

        sb.str("");
        sb.clear();
        sb.flush();
        iResult = 0;
        do
        {
            memset(recvbuf, 0, DEFAULT_BUFLEN);
            iResult = recv(client, recvbuf, DEFAULT_BUFLEN, 0); 
            if (iResult < 0)
            {
                errcode = WSAGetLastError();
                std::cout << "SERVER: Receive failed: " << GetErrorMessage(errcode) << std::endl;
                goto exitReceive;
            }
            else if (iResult > 0)
            {
                sb << recvbuf;
                if (iResult < DEFAULT_BUFLEN) // total mesaj bufferdan kucuk daha bekleme
                    break;
            }

        } while (iResult > 0);

        iResult = shutdown(client, SD_RECEIVE); // alış ile işimiz bitti
        if (iResult == SOCKET_ERROR)
        {
            errcode = WSAGetLastError();
            std::cout << "SERVER: Shutdown client socket failed " << GetErrorMessage(errcode) << std::endl;
        }

        GelenMesaj = sb.str();
        sb.str("");
        sb.clear();
        sb.flush();

        std::cout << "SERVER: Connection from " << ClientIp << std::endl;
        std::cout << "SERVER: Received: " << GelenMesaj << std::endl;

        if (GelenMesaj == "shutdown")
        {
            GidenMesaj = "BYE BYE";
        }
        else
        {
            temp = EvalMessage(GelenMesaj);
            if (!temp.assetid.empty())
            {
                temp.ftime = time(0);
                temp.clientip = ClientIp;
                temp.clienthost = ClientName;
              //  ItemQueue::AddItem(temp);
                thisObj->tt.AddItem(temp);
                sr.assetid = temp.assetid;
                sr.iitemstate = ItemState::queued;
                sr.success = true;
            }
            else
            {
                sr.errmessage = "Item info not valid";
                sr.iitemstate = ItemState::failed;
                sr.success = false;
            }

            GidenMesaj = sr.toJsonStr();
        }

        // send back message ====================================================================================
        iResult = 0;
        memset(sendbuf, 0, DEFAULT_BUFLEN);
        strcpy(sendbuf, GidenMesaj.c_str());
        iResult = send(client, sendbuf, strlen(sendbuf), 0);
        if (iResult == SOCKET_ERROR)
        {
            errcode = WSAGetLastError();
            std::cout << "Send back failed! ClientName: " << ClientName << " ClientIP: " << ClientIp << " Err:" << GetErrorMessage(errcode) << std::endl;

        }

    exitReceive:
        iResult = shutdown(client, SD_BOTH);
        if (iResult == SOCKET_ERROR)
        {
            errcode = WSAGetLastError();
            std::cout << "Shutdown client send/recv failed " << GetErrorMessage(errcode) << std::endl;
        }
        iResult = closesocket(client);
        if (iResult == SOCKET_ERROR)
        {
            errcode = WSAGetLastError();
            std::cout << "Close client socket failed " << GetErrorMessage(errcode) << std::endl;
        }
        if (GelenMesaj == "shutdown")
            break;
    }

exitListen:

    iResult = shutdown(server, SD_BOTH);
  //  if (iResult == SOCKET_ERROR)
  //  {
  //      errcode = WSAGetLastError();
  //      std::cout << "SERVER: Shutdown server socket failed " << GetErrorMessage(errcode) << std::endl;
  //  }

    iResult = closesocket(server);
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "SERVER: Close server socket failed" << GetErrorMessage(errcode) << std::endl;
    }

    WSACleanup();
    std::cout << "Stopping TCP server" << std::endl;
    pthread_exit(nullptr);
    return nullptr;
}

cItemInfo cSocket::EvalMessage(std::string pMsg)
{
    cItemInfo back;
    if (pMsg.empty())
        return back;
    if (!nlohmann::json::accept(pMsg))
    {
        std::cout << "not valid json for discovery parse" << std::endl;
        return back;
    }
    nlohmann::json j = nlohmann::json::parse(pMsg);
    if (j.is_discarded())
    {
        std::cout << "failed to parse discovery result" << std::endl;
        return back;
    }
    back.assetid = j["assetid"].get<std::string>();
    back.infile = j["infile"].get<std::string>();
    back.outfile = j["outfile"].get<std::string>();
    back.width = j["width"].get<int>();
    back.height = j["height"].get<int>();
    back.vbitratekb = j["vbitratekb"].get<int>();
    back.abitratekb = j["abitratekb"].get<int>();
    back.vencoder = j["vencoder"].get<std::string>();
    back.aencoder = j["aencoder"].get<std::string>();
    return back;
}

void cSocket::OnTranscodeComplete(TranscoderCBArgument pArg)
{
    std::cout << "AssetID: " << pArg.AssetId << " Success:" << pArg.Success << " ErrMsg:" << pArg.ErrMessage << " ProxyFile:" << pArg.ProxyFile << std::endl;
}

int cSocket::Start()
{
    ITranscoderCallBack *OnMesaj = nullptr;
    OnMesaj = new CTransCoderCallBack<cSocket>(this, &cSocket::OnTranscodeComplete);
    if (OnMesaj == nullptr)
    {
        std::cout << "ERR ===> Transcoder callback create failed!" << std::endl;
        return -1;
    }
    tt.SetCallBack(OnMesaj);
    tt.Start();


    int rv = 0;
    StopFlag = false;
    rv = pthread_create(&thHandle, nullptr, &cSocket::ServerThread, (void *)this);
    if (rv != 0)
    {
        std::cout << "ERR ===> Transcode thread create failed!" << std::endl;
    }

    rv = pthread_detach(thHandle);
    if (rv != 0)
    {
        std::cout << "ERR ===> Transcode thread detach failed!" << std::endl;
    }

    return rv;
}

int cSocket::Stop()
{
    int rv = 0;
    SendMsg("shutdown", "10.1.2.20", DEFAULT_PORT);
    tt.Stop();
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
    return back;
}

cSocket *cSocket::GetInstance()
{

    static Cleanup cleaner;
    if (thisObj == nullptr)
    {
        thisObj = new cSocket();
    }
    return thisObj;
}

cSocket::Cleanup::~Cleanup()
{
    if(thisObj)
        {
            delete thisObj;
            thisObj = nullptr;
        }
}


bool cSocket::SendMsg(std::string pMsg, std::string pDestIp, u_short pPort)
{
    bool back = false;
    int iResult;
    WSADATA wsaData;
    SOCKET ConnectSocket = INVALID_SOCKET;
    struct sockaddr_in clientService;
    int errcode = 0;

    int recvbuflen = DEFAULT_BUFLEN;
    char sendbuf[DEFAULT_BUFLEN] = {0};
    char recvbuf[DEFAULT_BUFLEN] = {0};
    int pStrLen = 0;
    int SendReceiveTimeout = 3000;

    pStrLen = strlen(pMsg.c_str());
    if (pStrLen < (DEFAULT_BUFLEN - 1))
        strcpy(sendbuf, pMsg.c_str());
    else
        memcpy(sendbuf, pMsg.c_str(), DEFAULT_BUFLEN - 1);

    //----------------------
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR)
    {
        std::cout << "CLIENT: Startup failed: " << std::endl;
        goto SendExit;
    }

    //----------------------
    // Create a SOCKET for connecting to server
    ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ConnectSocket == INVALID_SOCKET)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: Socket failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

    //----------------------
    // The sockaddr_in structure specifies the address family,
    // IP address, and port of the server to be connected to.
    clientService.sin_family = AF_INET;
    clientService.sin_addr.s_addr = inet_addr(pDestIp.c_str());
    clientService.sin_port = htons(pPort);

    iResult = setsockopt(ConnectSocket, SOL_SOCKET, SO_RCVTIMEO, (char *)&SendReceiveTimeout, sizeof(int));
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: set receive timeout failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

    iResult = setsockopt(ConnectSocket, SOL_SOCKET, SO_SNDTIMEO, (char *)&SendReceiveTimeout, sizeof(int));
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: set send timeout failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

    //----------------------
    // Connect to server.
    iResult = connect(ConnectSocket, (SOCKADDR *)&clientService, sizeof(clientService));
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: Connect failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

    iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: Send failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

    iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0)
    {
        std::cout << "CLIENT: Received: " << recvbuf << std::endl;
    }
    else if (iResult == 0)
        std::cout << "CLIENT: Connection closed" << std::endl;
    else
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: Receive failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

    back = true;

    iResult = shutdown(ConnectSocket, SD_BOTH);
    if (iResult == SOCKET_ERROR)
    {
        errcode = WSAGetLastError();
        std::cout << "CLIENT: Shutdown socket failed: " << GetErrorMessage(errcode) << std::endl;
        goto SendExit;
    }

SendExit:
    // close the socket
    iResult = closesocket(ConnectSocket);
    if (iResult == SOCKET_ERROR)
    {
        std::cout << "CLIENT: Close socket failed: " << GetErrorMessage(errcode) << std::endl;
    }
    WSACleanup();
    return back;
}