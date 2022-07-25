#include <iostream>
#include "utils.h"

using namespace std;
using namespace MyRPC;

int main() {
    char c = 'Y';
    cout << ntoh(hton(c)) << endl;
    short s =-90;
    cout << ntoh(hton(s)) << endl;
    float f =39.21;
    cout << ntoh(hton(f)) << endl;
    double d =-39.21;
    cout << ntoh(hton(d)) << endl;
    double e = 42.3;
    auto& re = e;
    cout << ntoh(hton(re)) << endl;
    cout << ntoh(hton(9+2)) << endl;
    return 0;
}


