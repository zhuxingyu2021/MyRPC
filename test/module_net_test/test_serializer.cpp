#include "net/serializer.h"
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <iostream>
#include "utils.h"

using namespace rapidjson;
using namespace std;
using namespace MyRPC;

int main() {
    float i =39.21;
    cout << ntoh(hton(i)) << endl;
    return 0;
}
