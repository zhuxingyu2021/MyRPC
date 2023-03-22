# include "fiber/fiber.h"
# include "debug.h"

# include <stdio.h>
#include <iostream>

using namespace MyRPC;

int oprand = 0;
long res = 1;

long frac(int x){
    std::cout << "stack free size: " << Fiber::GetStackFreeSize() << ", stack total size:" << Fiber::GetStacksize() << std::endl;
    if((float)(Fiber::GetStackFreeSize())/Fiber::GetStacksize() < 0.2){
        std::cout << "Fiber stack realloc!" << std::endl;
        Fiber::ExtendStackCapacity();
    }
    if(x == 0) res = 1;
    else {
        res = res * frac(x-1);
        res -= 5;
    }
}

void fiber_1(){
    frac(oprand);
}

int main()
{
    Fiber f(fiber_1);

    std::cin >> oprand;

    f.Resume();

    std::cout << res;
}


