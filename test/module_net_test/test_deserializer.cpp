#include "serialization/json_deserializer.h"
#include "serialization/json_serializer.h"
#include <unistd.h>
#include <iostream>

using namespace MyRPC;
using namespace std;

int main() {
    StringBuilder serialize_sb;
    JsonSerializer serializer(serialize_sb);

    StringBuilder sb1;
    sb1.Append("{\n"
               "    \"first\": [\n"
               "        5076,\n"
               "        -1,\n"
               "        -293,\n"
               "        -923,\n"
               "        1817\n"
               "    ],\n"
               "    \"second\": [\n"
               "        212023,\n"
               "        -19\n"
               "    ],\n"
               "    \"third\": [\n"
               "        29,\n"
               "        892,\n"
               "        -12,\n"
               "        81\n"
               "    ]\n"
               "}");
    StringBuffer buf1 = sb1.Concat();
    JsonDeserializer des1(buf1);
    std::map<string, vector<int>> map1;
    des1.Load(map1);
    serializer.Save(map1);
    StringBuffer bufs1 = serialize_sb.Concat();
    std::cout << bufs1.ToString() << std::endl <<"==========================================================" << std::endl;
    serialize_sb.Clear();

    StringBuilder sb2;
    sb2.Append(
            "[\n\r"
            "  {\n\r"
            "    \"firstName\": \"Brett\",\n\r"
            "    \"email\": \"brett@newInstance.com\"\n\r"
            "  },\n"
            "  {\n"
            "    \"firstName\": \"Jason\",\n\r"
            "    \"email\": \"jason@servlets.com\"\n\r"
            "  }\n\r"
            "]");
    StringBuffer buf2 = sb2.Concat();
    JsonDeserializer des2(buf2);
    std::vector<std::map<std::string, std::string>> vec;
    des2.Load(vec);
    serializer.Save(vec);
    StringBuffer bufs2 = serialize_sb.Concat();
    std::cout <<bufs2.ToString() << std::endl <<"==========================================================" << std::endl;
    serialize_sb.Clear();


    StringBuilder sb3;
    sb3.Append("[\n"
               "\t{\n"
               "\t\t\"key\": [\n"
               "\t\t\t12,\n"
               "\t\t\t13,\n"
               "\t\t\t2,\n"
               "\t\t\t6\n"
               "\t\t],\n"
               "\t\t\"value\": [\n"
               "\t\t\t11,\n"
               "\t\t\t8,\n"
               "\t\t\t9,\n"
               "\t\t\t0\n"
               "\t\t]\n"
               "\t}\n"
               "]");
    StringBuffer buf3 = sb3.Concat();
    JsonDeserializer des3(buf3);
    std::pair<vector<int>, vector<int>> pair1;
    des3.Load(pair1);
    serializer.Save(pair1);
    StringBuffer bufs3 = serialize_sb.Concat();
    std::cout << bufs3.ToString() << std::endl <<"==========================================================" << std::endl;
    serialize_sb.Clear();


    StringBuilder sb4;
    sb4.Append("[\n"
               "  {\n"
               "    \"item1_1\": [\n"
               "      5076,\n"
               "      -1,\n"
               "      -293,\n"
               "      -923,\n"
               "      1817\n"
               "    ],\n"
               "    \"item1_2\": [\n"
               "      212023,\n"
               "      -19\n"
               "    ],\n"
               "    \"item1_3\": [\n"
               "      29,\n"
               "      892,\n"
               "      -12,\n"
               "      81\n"
               "    ]\n"
               "  },\n"
               "  null,\n"
               "  {\n"
               "    \"item3\": [\n"
               "      39,\n"
               "      -9,\n"
               "      -12,\n"
               "      -29\n"
               "    ],\n"
               "    \"item4\": [\n"
               "      -2938422,\n"
               "      -9,\n"
               "      -12,\n"
               "      -29\n"
               "    ]\n"
               "  }\n"
               "]");
    StringBuffer buf4 = sb4.Concat();
    JsonDeserializer des4(buf4);
    std::vector<std::optional<shared_ptr<std::map<std::string, vector<int>>>>> vec2;
    des4.Load(vec2);
    serializer.Save(vec2);
    StringBuffer bufs4 = serialize_sb.Concat();
    std::cout << bufs4.ToString() << std::endl <<"==========================================================" << std::endl;
    serialize_sb.Clear();


    StringBuilder sb5;
    sb5.Append("[\n"
               "  232.111,\n"
               "  19,\n"
               "  [\n"
               "    32,\n"
               "    901,\n"
               "    12,\n"
               "    29,\n"
               "    -323\n"
               "  ],\n"
               "  \"Hello!\"\n"
               "]");
    StringBuffer buf5 = sb5.Concat();
    JsonDeserializer des5(buf5);
    tuple<double, int, vector<int>, string> tuple1;
    des5.Load(tuple1);
    serializer.Save(tuple1);
    StringBuffer bufs5 = serialize_sb.Concat();
    std::cout << bufs5.ToString() << std::endl <<"==========================================================" << std::endl;
    serialize_sb.Clear();


    StringBuilder sb6;
    sb6.Append("[{\"_id\":\"62e1248a000b1b36a3e785f4\",\"age\":24,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Allison Spence\",\"gender\":\"male\",\"company\":\"BUZZOPIA\",\"email\":\"allisonspence@buzzopia.com\",\"phone\":\"+1 (956) 549-3729\",\"address\":\"460 Gerald Court, Trucksville, Nevada, 154\",\"about\":\"Laboris aliquip nisi ullamco ad eiusmod et consequat cupidatat. Incididunt aliquip labore quis amet veniam. Mollit est quis officia ullamco occaecat consequat consectetur duis. Tempor minim exercitation mollit nulla culpa nostrud laboris sit enim ea cillum aliquip.\\r\\n\",\"tags\":[\"ut\",\"Lorem\",\"veniam\",\"id\",\"excepteur\",\"tempor\",\"est\"],\"friends\":[{\"key\":0,\"value\":\"Myrtle Horton\"},{\"key\":1,\"value\":\"Richard Frederick\"},{\"key\":2,\"value\":\"Dora Mack\"}]},{\"_id\":\"62e1248a804e0853c5cdf7e1\",\"age\":38,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Randi Barr\",\"gender\":\"female\",\"company\":\"AFFLUEX\",\"email\":\"randibarr@affluex.com\",\"phone\":\"+1 (835) 509-2423\",\"address\":\"140 Bushwick Avenue, Takilma, Alaska, 9947\",\"about\":\"Anim magna exercitation fugiat duis sunt enim. Incididunt laboris nostrud ipsum esse dolor sint nisi adipisicing et laboris excepteur. Occaecat eiusmod ut labore tempor cillum. Occaecat dolor mollit excepteur labore proident.\\r\\n\",\"tags\":[\"nulla\",\"excepteur\",\"ad\",\"ex\",\"incididunt\",\"enim\",\"quis\"],\"friends\":[{\"key\":0,\"value\":\"Leonor Pitts\"},{\"key\":1,\"value\":\"Robbins Prince\"},{\"key\":2,\"value\":\"Debora Warren\"}]},{\"_id\":\"62e1248a27346996fb025e66\",\"age\":24,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Allie Hood\",\"gender\":\"female\",\"company\":\"BLANET\",\"email\":\"alliehood@blanet.com\",\"phone\":\"+1 (876) 538-2712\",\"address\":\"105 Charles Place, Bath, South Dakota, 2936\",\"about\":\"Labore ad excepteur commodo sint. Deserunt ut et aliqua cillum fugiat proident veniam. Anim incididunt veniam labore esse nostrud voluptate officia ex. Nisi veniam in sit officia labore exercitation culpa ut ullamco qui. Velit ex cillum irure eiusmod elit.\\r\\n\",\"tags\":[\"id\",\"minim\",\"in\",\"cillum\",\"excepteur\",\"cupidatat\",\"ad\"],\"friends\":[{\"key\":0,\"value\":\"Rosalie Vance\"},{\"key\":1,\"value\":\"Pearlie Wood\"},{\"key\":2,\"value\":\"Wilcox Simpson\"}]},{\"_id\":\"62e1248a68fa9c5301c45c36\",\"age\":36,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Aileen Guthrie\",\"gender\":\"female\",\"company\":\"ZORROMOP\",\"email\":\"aileenguthrie@zorromop.com\",\"phone\":\"+1 (832) 521-2577\",\"address\":\"960 Vanderbilt Street, Clayville, Minnesota, 6560\",\"about\":\"Consectetur nulla deserunt veniam enim irure. Culpa proident cupidatat esse laborum amet non deserunt commodo et quis irure. Nostrud consectetur in pariatur aliqua do anim duis veniam culpa. Minim non amet qui labore elit exercitation cupidatat exercitation laboris ea voluptate aute. Officia adipisicing irure eiusmod et ea voluptate do incididunt.\\r\\n\",\"tags\":[\"excepteur\",\"et\",\"nisi\",\"est\",\"ullamco\",\"et\",\"velit\"],\"friends\":[{\"key\":0,\"value\":\"Silvia Mcintyre\"},{\"key\":1,\"value\":\"Helen Oconnor\"},{\"key\":2,\"value\":\"Kasey Pugh\"}]},{\"_id\":\"62e1248a388d03384b2997fb\",\"age\":35,\"picture\":\"http://placehold.it/32x32\",\"name\":\"Freda Copeland\",\"gender\":\"female\",\"company\":\"PIGZART\",\"email\":\"fredacopeland@pigzart.com\",\"phone\":\"+1 (834) 418-2161\",\"address\":\"145 Tiffany Place, Carlos, American Samoa, 2949\",\"about\":\"Tempor ea nisi consequat voluptate consectetur dolore id elit irure laboris consectetur eiusmod proident. Et fugiat magna eu dolore ut magna sit ea. Deserunt nostrud cillum adipisicing do incididunt excepteur laboris id voluptate dolor ad qui veniam irure. Elit deserunt quis pariatur minim ad mollit non ea non deserunt laborum. Aliqua magna enim ipsum in aliqua esse. Veniam consectetur officia cupidatat quis ut Lorem nulla elit. Proident consequat incididunt veniam in eu dolore aliquip duis laboris laborum dolore aute eiusmod.\\r\\n\",\"tags\":[\"non\",\"reprehenderit\",\"anim\",\"incididunt\",\"sint\",\"et\",\"amet\"],\"friends\":[{\"key\":0,\"value\":\"Deann Pennington\"},{\"key\":1,\"value\":\"Keller Flowers\"},{\"key\":2,\"value\":\"Cotton Glenn\"}]}]");
    StringBuffer buf6 = sb6.Concat();
    JsonDeserializer des6(buf6);

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

    des6.Load(persons);

    return 0;
}