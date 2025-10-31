#include "timestamp.hpp"
#include <iostream>

#include <set>

using namespace std;
using namespace furina;

int main(){
    cout << Timestamp::now().getNowTime() << endl;
    cout << Timestamp::nowTimeMs() << endl;

    auto cmp = [](int lhs, int rhs)->bool{
        return lhs < rhs;
    };

    set<int, decltype(cmp)> s;
    s.insert(1);
    s.insert(2);
    s.insert(3);

    for(auto it = s.begin(); it != s.end(); it++){
        cout << *it << endl;
    }

    return 0;
}