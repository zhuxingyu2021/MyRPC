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
    {
        rapidjson::StringBuffer s;
        JsonSerializer serializer(s);

        vector<int> vec1 = {32, 901, 12, 29, -323};
        serializer << vec1;
        std::cout << s.GetString() << std::endl;
    }
    {
        rapidjson::StringBuffer s;
        JsonSerializer serializer(s);

        array<string, 3> array1 = {"12o", "hELLO!", "Yeah!"};
        serializer << array1;
        std::cout << s.GetString() << std::endl;
    }
    {
        rapidjson::StringBuffer s;
        JsonSerializer serializer(s);
        std::unordered_map<float, string> map1 = {{9.23,   "12"},
                                                  {92.19,  "2opw!"},
                                                  {68.238, std::to_string(3.192)}};
        serializer << map1;
        std::cout << s.GetString() << std::endl;
    }
    {
        rapidjson::StringBuffer s;
        JsonSerializer serializer(s);

        std::pair<vector<int>, vector<int>> pair1;
        pair1.first.push_back(12);
        pair1.first.push_back(13);
        pair1.first.push_back(2);
        pair1.first.push_back(6);
        pair1.second.push_back(11);
        pair1.second.push_back(8);
        pair1.second.push_back(9);
        pair1.second.push_back(0);

        serializer<<pair1;
        std::cout << s.GetString() << std::endl;
    }
    {
        rapidjson::StringBuffer s;
        JsonSerializer serializer(s);
        vector<int> vec1 = {32, 901, 12, 29, -323};
        string hello = "Hello!";
        tuple<double, int, vector<int>, string> tuple1 = std::make_tuple(232.111, 19, std::move(vec1), std::move(hello));
        serializer<<tuple1;
        std::cout << s.GetString() << std::endl;
    }
    {
        rapidjson::StringBuffer s;
        JsonSerializer serializer(s);
        set<string> set1= {"323", "dqwd", "ca", "ld"};
        serializer<<set1;
        std::cout << s.GetString() << std::endl;
    }


}
