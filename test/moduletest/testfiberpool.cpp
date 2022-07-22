#include "fiberpool.h"
#include "fiber.h"
#include <iostream>
#include <vector>
#include "logger.h"

using namespace MyRPC;

int main(){
    FiberPool fp(8);
    fp.Start();

    std::vector<FiberPool::FiberController> v;

    for(int i = 0; i < 50; i++){
        v.push_back(fp.Run([](){
            Logger::info("Hello World! From fiber {}", Fiber::GetCurrentId());
        }));
    }

    for(auto &f : v){
        f.Join();
    }

    fp.Stop();
    return 0;
}

