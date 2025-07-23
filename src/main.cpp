#include <iostream>
#include "random/TestRandom.h"

int main() {
    std::cout << "Test Cpp Project" << "\n";
    std::cout << "Hello, VSCode C++ on macOS!" << std::endl;

    initRandomNumByCStyle();
    initRandomNumByCpp("int");
    initRandomNumByCpp("real");
    initRandomNumByCpp("normal");

    return 0;
}

