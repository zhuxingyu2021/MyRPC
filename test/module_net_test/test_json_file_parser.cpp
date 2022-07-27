#include "net/deserializer.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "utils.h"
#include <chrono>
#include <iostream>

using namespace MyRPC;

struct Person{
    std::string _id;
    int age;
    std::string picture;
    std::string name;
    std::string gender;
    std::string company;
    std::string email;
    std::string phone;
    std::string address;
    std::string about;

    std::vector<std::string> tags;
    std::map<int,std::string> friends;

    LOAD_BEGIN
        LOAD_ITEM(_id)
        LOAD_ITEM(age)
        LOAD_ITEM(picture)
        LOAD_ITEM(name)
        LOAD_ITEM(gender)
        LOAD_ITEM(company)
        LOAD_ITEM(email)
        LOAD_ITEM(phone)
        LOAD_ITEM(address)
        LOAD_ITEM(about)
        LOAD_ITEM(tags)
        LOAD_ITEM(friends)
    LOAD_END

};

int main(int argc, char** argv)
{
    if(argc < 2){
        printf("Usage: %s <json_file>\n", argv[0]);
        return -1;
    }
    int fd = open(argv[1], O_RDONLY);
    if(fd > 0){
        StringBuffer s;
        char buf[4096];
        while(true){
            int sz = read(fd, buf, sizeof(buf));
            if(sz <= 0){
                break;
            }
            auto new_sz = strtrim<' ', '\t', '\n', '\r'>(buf, sz);
            s.Write(buf, new_sz);
        }

        std::vector<Person> persons;
        Deserializer d(s);

        auto start = std::chrono::system_clock::now();
        d.Load(persons);

        auto end = std::chrono::system_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        double second = double(duration.count()) * std::chrono::microseconds::period::num / std::chrono::microseconds::period::den;

        std::cout << "Time elapsed " << second << "s" << std::endl;

        close(fd);
    }else{
        printf("open %s failed\n", argv[1]);
        return -1;
    }
}

