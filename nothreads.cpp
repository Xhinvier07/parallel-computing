#include <iostream>
using namespace std;

string words[4] = { "FEU", "INSTITUTE", "OF", "TECHNOLOGY" };

void print();


int main() {
    print();
    return 0;
}

void print() {
    for (int i = 0; i < 1000; i++) {
        for (int j = 0; j < 4; j++) {
            cout << words[j] << " ";
        }
    }

    cout << endl;
    cout << endl;
    cout << "Jansen Jhoel G. Moral";
}