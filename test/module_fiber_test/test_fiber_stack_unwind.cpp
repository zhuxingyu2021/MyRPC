# include "fiber/fiber.h"
# include "debug.h"

# include <stdio.h>
#include <iostream>

using namespace MyRPC;

class A{
public:
    ~A(){
        std::cout << "A Deconstruction!" << std::endl;
    }

    void func(){
        std::cout << "print here!" << std::endl;
    }
};

std::shared_ptr<A> g_ptr;

void fiber_1(){
    auto s = g_ptr;
    s->func();
    Fiber::Suspend();
}

int main()
{
    g_ptr = std::make_shared<A>();
    Fiber *f = new Fiber(fiber_1);
    f->Resume();
    std::cout << g_ptr.use_count() << std::endl;

    delete f; // stack unwind
    std::cout << g_ptr.use_count() << std::endl;
}

