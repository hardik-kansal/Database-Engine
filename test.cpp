#include <iostream>

int main() {
    std::cout << "\033[2J\033[1;1H";
    std::cout << "Hello after clear!" << std::endl;
    return 0;
}