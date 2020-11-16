#include <iostream>
#include "HuffmanTree.h"

using namespace std;

int main() {

    HuffmanTree<char> enc{"test.txt", "test.txt.enc"};
    enc.encode();

    HuffmanTree<char> dec{"test.txt.enc", "test.txt.dec"};
    dec.decode();

    return 0;
}
