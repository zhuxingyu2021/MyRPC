#include "net/serializer.h"
#include <iostream>
#include "utils.h"
#include <unistd.h>

using namespace std;
using namespace MyRPC;

int main() {
    StringBuffer s;
    JsonSerializer serializer(s);

    vector<int> vec1 = {32, 901, 12, 29, -323};
    serializer.Save(vec1);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    array<string, 3> array1 = {"12o", "hELLO!", "Yeah!"};
    serializer.Save(array1);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    std::unordered_map<float, string> map1 = {{9.23,   "12"},
                                              {92.19,  "2opw!"},
                                              {68.238, std::to_string(3.192)}};
    serializer.Save(map1);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    std::pair<vector<int>, vector<int>> pair1;
    pair1.first.push_back(12);
    pair1.first.push_back(13);
    pair1.first.push_back(2);
    pair1.first.push_back(6);
    pair1.second.push_back(11);
    pair1.second.push_back(8);
    pair1.second.push_back(9);
    pair1.second.push_back(0);

    serializer.Save(pair1);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    string hello = "Hello!";
    tuple<double, int, vector<int>, string> tuple1 = std::make_tuple(232.111, 19, std::move(vec1), std::move(hello));
    serializer.Save(tuple1);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    set<string> set1= {"323", "dqwd", "ca", "ld"};
    serializer.Save(set1);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();


    std::map<string, vector<int>> map2 = {{"你好", {29, 892, -12, 81}},
                                          {"A", {212023, -19}},
                                          {"2os2", {5076, -1, -293, -923, 1817}}};
    std::map<string, vector<int>> map3 = {{"32dw", {39, -9, -12, -29}},
                                          {"！No", {-2938422, -9, -12, -29}}};


    serializer.Save(map2);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    vector<optional<std::shared_ptr<std::map<std::string, vector<int>>>>> vec2;

    vec2.emplace_back(new std::map<std::string , vector<int>>(map2));
    vec2.push_back(nullopt);
    vec2.emplace_back(new std::map<std::string, vector<int>>(map3));
    serializer.Save(vec2);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

    s.Clear();

    struct Table{
        int id;
        std::string name;

        SAVE_BEGIN
            SAVE_ITEM(id)
            SAVE_ITEM(name)
        SAVE_END

    };
    Table t = {0, "Xiao Ming"};

    serializer.Save(t);
    s.WriteFile(STDIN_FILENO);
    std::cout << std::endl <<"==========================================================" << std::endl;

}
