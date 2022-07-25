#include "net/deserializer.h"
#include "net/serializer.h"
#include <unistd.h>
#include <iostream>

using namespace MyRPC;
using namespace std;

int main() {
    StringBuffer buffer1(1000);
    Deserializer deserializer(buffer1);
    StringBuffer buffer2(1000);
    Serializer serializer(buffer2);
    std::string str1 = "{\"2os2\":[5076,-1,-293,-923,1817],\"A\":[212023,-19],\"你好\":[29,892,-12,81]}";
    buffer1.Write(str1.c_str(), str1.size());

    std::map<string, vector<int>> map1;
    deserializer >> map1;
    serializer << map1;
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    buffer2.Clear();
    std::cout << "==========================================================" << std::endl;

    std::string str2 = "[{\"firstName\":\"Brett\",\"email\":\"brett@newInstance.com\"},{\"firstName\":\"Jason\",\"email\":\"jason@servlets.com\"}]";
    buffer1.Write(str2.c_str(), str2.size());

    std::vector<std::map<std::string, std::string>> vec;
    deserializer >> vec;
    serializer << vec;
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    buffer2.Clear();

    std::cout << "==========================================================" << std::endl;

    std::string str3 = "{\"[12,13,2,6]\":[11,8,9,0]}";
    buffer1.Write(str3.c_str(), str3.size());

    std::pair<vector<int>, vector<int>> pair1;
    deserializer >> pair1;
    serializer << pair1;
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    buffer2.Clear();

    std::cout << "==========================================================" << std::endl;

    std::string str4 = "[{\"\"2os2\"\":[5076,-1,-293,-923,1817],\"\"A\"\":[212023,-19],\"\"你好\"\":[29,892,-12,81]},null,{\"\"32dw\"\":[39,-9,-12,-29],\"\"！No\"\":[-2938422,-9,-12,-29]}]";
    buffer1.Write(str4.c_str(), str4.size());

    std::vector<std::optional<shared_ptr<std::map<std::shared_ptr<std::string>, vector<int>>>>> vec2;
    deserializer >> vec2;
    serializer << vec2;
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    buffer2.Clear();

    std::cout << "==========================================================" << std::endl;

    return 0;
}