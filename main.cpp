#include <iostream>
#include "Socket.hpp"


void Test()
{
    cSocket* s = cSocket::GetInstance();    
    s->Start();
    Sleep(3000);
    s->SendMsg("shutdown","10.1.2.20",DEFAULT_PORT);
}

int main()
{
    
    Test();
     system("pause");
    return 0;
}