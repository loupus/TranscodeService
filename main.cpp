#include <iostream>
#include "Socket.hpp"


void Test()
{
    cSocket* s = cSocket::GetInstance();    
    s->Start();
   // s->SendMsg("shutdown","127.0.0.1",27015);
}

int main()
{
    Test();
     system("pause");
    return 0;
}