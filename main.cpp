#include <iostream>
#include "Globals.hpp"
/*
#include "Socket.hpp"
#include "nlohmann/json.hpp"
*/
/*
void Test()
{
    cSocket *s = cSocket::GetInstance();
    s->Start();
    Sleep(1000);

    std::string testItem = "";
    nlohmann::json j = nlohmann::json{
        {"assetid", "alomelo"},
        {"infile", "\\\\hansel.sistem.turkmedya.local\\AGENCIES\\20211215_3_51313908_71613460_SD.mp4"},
        {"outfile", "\\\\hansel.sistem.turkmedya.local\\AGENCIES\\20211215_3_51313908_71613460_SD_Proxy.mp4"},
        {"width", 854},
        {"height", 480},
        {"vbitratekb", 600},
        {"abitratekb", 128},
        {"vencoder", "H264"},
        {"aencoder", "copy"}};
    testItem = j.dump();
    s->SendMsg(testItem, "10.1.2.20", DEFAULT_PORT);
    Sleep(30000);

    s->Stop();
}
*/

void Menu()
{
 
    std::string logo = R"(
    +-+ +-+ +-+ +-+ +-+   +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ 
    |N| |e| |s| |e| |s|   |T| |r| |a| |n| |s| |c| |o| |d| |e|   
    +-+ +-+ +-+ +-+ +-+   +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+ +-+)";

    std::cout << "========================================================================" << std::endl;
    std::cout << logo << std::endl;
    std::cout << "\tVersion 1.1" << std::endl;
    std::cout << std::endl;
    std::cout << "\t exit              => to exit program" << std::endl;
    std::cout << "\t start             => start loop" << std::endl;
    std::cout << "\t stop              => stop loop" << std::endl;
    std::cout << "\t menu              => bring menu" << std::endl;
    std::cout <<  std::endl;
    std::cout  << std::endl;
    std::cout << "Hakan Soyalp soyalpha@gmail.com" << std::endl;
    std::cout << "========================================================================" << std::endl;

}

int main()
{
    HWND console = GetConsoleWindow();
	RECT ConsoleRect;
    int width = 600;
    int height = 600;
    COORD k;
    k.X = width;
    k.Y = height;
    SetConsoleTitle("Neses Transcode Service");
    SetConsoleScreenBufferSize(console, k);
	GetWindowRect(console, &ConsoleRect); 
    MoveWindow(console, ConsoleRect.left, ConsoleRect.top, 800, 600, TRUE);
    Menu();

    system("pause");
    return 0; 
}