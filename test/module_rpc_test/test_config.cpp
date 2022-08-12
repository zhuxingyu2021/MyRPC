#include "rpc/config.h"

#include <memory>

using namespace MyRPC;
using namespace std;

int main(){
    Config::ptr config = make_shared<Config>();
    config->SaveToJson("test.json");

    Config::ptr config2 = Config::LoadFromJson("test.json");

    return 0;
}
