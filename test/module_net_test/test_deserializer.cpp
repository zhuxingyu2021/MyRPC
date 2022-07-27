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
    deserializer.Load(map1);
    serializer.Save(map1);
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    deserializer.Reset();
    buffer2.Clear();
    std::cout << std::endl <<"==========================================================" << std::endl;

    std::string str2 = "[{\"firstName\":\"Brett\",\"email\":\"brett@newInstance.com\"},{\"firstName\":\"Jason\",\"email\":\"jason@servlets.com\"}]";
    buffer1.Write(str2.c_str(), str2.size());

    std::vector<std::map<std::string, std::string>> vec;
    deserializer.Load(vec);
    serializer.Save(vec);
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    deserializer.Reset();
    buffer2.Clear();

    std::cout << std::endl <<"==========================================================" << std::endl;

    std::string str3 = "[{\"key\":[12,13,2,6],\"value\":[11,8,9,0]}]";
    buffer1.Write(str3.c_str(), str3.size());

    std::pair<vector<int>, vector<int>> pair1;
    deserializer.Load(pair1);
    serializer.Save(pair1);
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    deserializer.Reset();
    buffer2.Clear();

    std::cout << std::endl <<"==========================================================" << std::endl;

    std::string str4 = "[{\"2os2\":[5076,-1,-293,-923,1817],\"A\":[212023,-19],\"你好\":[29,892,-12,81]},null,{\"32dw\":[39,-9,-12,-29],\"！No\":[-2938422,-9,-12,-29]}]";
    buffer1.Write(str4.c_str(), str4.size());

    std::vector<std::optional<shared_ptr<std::map<std::string, vector<int>>>>> vec2;
    deserializer.Load(vec2);
    serializer.Save(vec2);
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    deserializer.Reset();
    buffer2.Clear();

    std::cout << std::endl <<"==========================================================" << std::endl;

    std::string str5 = "[232.111000,19,[32,901,12,29,-323],\"Hello!\"]";
    buffer1.Write(str5.c_str(), str5.size());

    tuple<double, int, vector<int>, string> tuple1;
    deserializer.Load(tuple1);
    serializer.Save(tuple1);
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    deserializer.Reset();
    buffer2.Clear();

    std::cout << std::endl <<"==========================================================" << std::endl;
    std::string str6 = "{\"id\":0,\"name\":\"Xiao Ming\"}";
    buffer1.Write(str6.c_str(), str6.size());

    struct Table{
        int id;
        std::string name;

        SAVE_BEGIN
            SAVE_ITEM(id)
            SAVE_ITEM(name)
        SAVE_END

        LOAD_BEGIN
            LOAD_ITEM(id)
            LOAD_ITEM(name)
        LOAD_END

    };

    Table table1;
    deserializer.Load(table1);
    serializer.Save(table1);
    buffer2.WriteFile(STDOUT_FILENO);
    buffer1.Clear();
    deserializer.Reset();
    buffer2.Clear();
    std::cout << std::endl <<"==========================================================" << std::endl;

    return 0;
}