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

    std::string str7 = "[{\"_id\":\"62e1248a000b1b36a3e785f4\",\"age\":24,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Allison Spence\",\"gender\":\"male\",\"company\":\"BUZZOPIA\",\"email\":\"allisonspence@buzzopia.com\",\"phone\":\"+1 (956) 549-3729\",\"address\":\"460 Gerald Court, Trucksville, Nevada, 154\",\"about\":\"Laboris aliquip nisi ullamco ad eiusmod et consequat cupidatat. Incididunt aliquip labore quis amet veniam. Mollit est quis officia ullamco occaecat consequat consectetur duis. Tempor minim exercitation mollit nulla culpa nostrud laboris sit enim ea cillum aliquip.\\r\\n\",\"tags\":[\"ut\",\"Lorem\",\"veniam\",\"id\",\"excepteur\",\"tempor\",\"est\"],\"friends\":[{\"key\":0,\"value\":\"Myrtle Horton\"},{\"key\":1,\"value\":\"Richard Frederick\"},{\"key\":2,\"value\":\"Dora Mack\"}]},{\"_id\":\"62e1248a804e0853c5cdf7e1\",\"age\":38,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Randi Barr\",\"gender\":\"female\",\"company\":\"AFFLUEX\",\"email\":\"randibarr@affluex.com\",\"phone\":\"+1 (835) 509-2423\",\"address\":\"140 Bushwick Avenue, Takilma, Alaska, 9947\",\"about\":\"Anim magna exercitation fugiat duis sunt enim. Incididunt laboris nostrud ipsum esse dolor sint nisi adipisicing et laboris excepteur. Occaecat eiusmod ut labore tempor cillum. Occaecat dolor mollit excepteur labore proident.\\r\\n\",\"tags\":[\"nulla\",\"excepteur\",\"ad\",\"ex\",\"incididunt\",\"enim\",\"quis\"],\"friends\":[{\"key\":0,\"value\":\"Leonor Pitts\"},{\"key\":1,\"value\":\"Robbins Prince\"},{\"key\":2,\"value\":\"Debora Warren\"}]},{\"_id\":\"62e1248a27346996fb025e66\",\"age\":24,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Allie Hood\",\"gender\":\"female\",\"company\":\"BLANET\",\"email\":\"alliehood@blanet.com\",\"phone\":\"+1 (876) 538-2712\",\"address\":\"105 Charles Place, Bath, South Dakota, 2936\",\"about\":\"Labore ad excepteur commodo sint. Deserunt ut et aliqua cillum fugiat proident veniam. Anim incididunt veniam labore esse nostrud voluptate officia ex. Nisi veniam in sit officia labore exercitation culpa ut ullamco qui. Velit ex cillum irure eiusmod elit.\\r\\n\",\"tags\":[\"id\",\"minim\",\"in\",\"cillum\",\"excepteur\",\"cupidatat\",\"ad\"],\"friends\":[{\"key\":0,\"value\":\"Rosalie Vance\"},{\"key\":1,\"value\":\"Pearlie Wood\"},{\"key\":2,\"value\":\"Wilcox Simpson\"}]},{\"_id\":\"62e1248a68fa9c5301c45c36\",\"age\":36,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Aileen Guthrie\",\"gender\":\"female\",\"company\":\"ZORROMOP\",\"email\":\"aileenguthrie@zorromop.com\",\"phone\":\"+1 (832) 521-2577\",\"address\":\"960 Vanderbilt Street, Clayville, Minnesota, 6560\",\"about\":\"Consectetur nulla deserunt veniam enim irure. Culpa proident cupidatat esse laborum amet non deserunt commodo et quis irure. Nostrud consectetur in pariatur aliqua do anim duis veniam culpa. Minim non amet qui labore elit exercitation cupidatat exercitation laboris ea voluptate aute. Officia adipisicing irure eiusmod et ea voluptate do incididunt.\\r\\n\",\"tags\":[\"excepteur\",\"et\",\"nisi\",\"est\",\"ullamco\",\"et\",\"velit\"],\"friends\":[{\"key\":0,\"value\":\"Silvia Mcintyre\"},{\"key\":1,\"value\":\"Helen Oconnor\"},{\"key\":2,\"value\":\"Kasey Pugh\"}]},{\"_id\":\"62e1248a388d03384b2997fb\",\"age\":35,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Freda Copeland\",\"gender\":\"female\",\"company\":\"PIGZART\",\"email\":\"fredacopeland@pigzart.com\",\"phone\":\"+1 (834) 418-2161\",\"address\":\"145 Tiffany Place, Carlos, American Samoa, 2949\",\"about\":\"Tempor ea nisi consequat voluptate consectetur dolore id elit irure laboris consectetur eiusmod proident. Et fugiat magna eu dolore ut magna sit ea. Deserunt nostrud cillum adipisicing do incididunt excepteur laboris id voluptate dolor ad qui veniam irure. Elit deserunt quis pariatur minim ad mollit non ea non deserunt laborum. Aliqua magna enim ipsum in aliqua esse. Veniam consectetur officia cupidatat quis ut Lorem nulla elit. Proident consequat incididunt veniam in eu dolore aliquip duis laboris laborum dolore aute eiusmod.\\r\\n\",\"tags\":[\"non\",\"reprehenderit\",\"anim\",\"incididunt\",\"sint\",\"et\",\"amet\"],\"friends\":[{\"key\":0,\"value\":\"Deann Pennington\"},{\"key\":1,\"value\":\"Keller Flowers\"},{\"key\":2,\"value\":\"Cotton Glenn\"}]}]";
    buffer1.Write(str7.c_str(), str7.size());

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

        SAVE_BEGIN
            SAVE_ITEM(_id)
            SAVE_ITEM(age)
            SAVE_ITEM(picture)
            SAVE_ITEM(name)
            SAVE_ITEM(gender)
            SAVE_ITEM(company)
            SAVE_ITEM(email)
            SAVE_ITEM(phone)
            SAVE_ITEM(address)
            SAVE_ITEM(about)
            SAVE_ITEM(tags)
            SAVE_ITEM(friends)
        SAVE_END

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

    std::vector<Person> persons;

    deserializer.Load(persons);

    return 0;
}