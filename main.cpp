#include <iostream>
#include "Socket.hpp"

void Test()
{
    cSocket *s = cSocket::GetInstance();
    s->Start();
    Sleep(1000);

    std::string testItem = "";
    nlohmann::json j = nlohmann::json{
        {"assetid", "alomelo"},
        {"infile", "\\\\hansel.sistem.turkmedya.local\\AGENCIES\\20211030_3_50666471_70171793_SD.mp4"},
        {"outfile", "\\\\hansel.sistem.turkmedya.local\\AGENCIES\\20211030_3_50666471_70171793_SD_Proxy.mp4"},
        {"width", 854},
        {"height", 480},
        {"vbitratekb", 600},
        {"abitratekb", 128},
        {"vencoder", "H264"},
        {"aencoder", "copy"}};
    testItem = j.dump();
    s->SendMsg(testItem, "10.1.2.20", DEFAULT_PORT);
    Sleep(3000);

    s->SendMsg("shutdown", "10.1.2.20", DEFAULT_PORT);
}

int main()
{

    Test();
    system("pause");
    return 0;
}