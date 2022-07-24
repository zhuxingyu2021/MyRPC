# include "fiber/fiber.h"
# include "macro.h"

# include <iostream>

using namespace MyRPC;

void fiber_1(){
    std::cout << "Fiber1 started!" << std::endl;
    std::cout << "Ready to Suspend 1" << std::endl;
    Fiber::Suspend();
    std::cout << "Ready to Suspend 2" << std::endl;
    Fiber::Suspend();
}

int main()
{
    Fiber f(fiber_1);
    f.Resume();
    std::cout << "Ready to Resume 1" << std::endl;
    f.Resume();
    std::cout << "Ready to Resume 2" << std::endl;
    f.Resume();
    std::cout << "Ready to Resume 3" << std::endl;
    f.Resume();

}

