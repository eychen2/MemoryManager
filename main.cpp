#include <iostream>
#include <stdlib.h>
#include <sstream>
#include "MemoryManager.h"
struct test
{
    bool isTrue=false;
    int offset=-1;
};
using namespace std;
int main() {
    MemoryManager memoryManager(4,bestFit);
    memoryManager.initialize(12);
    memoryManager.allocate(13);
    memoryManager.allocate(12);
    memoryManager.allocate(14);
    uint16_t* list = static_cast<uint16_t*>(memoryManager.getList());
    cout<<list[0]<<endl;
    uint16_t listLength = *list++;
    uint16_t bytesPerEntry = 2;
    uint16_t entriesInList = listLength * bytesPerEntry;
    std::cout << "Got:"  << std::endl;
    std::cout << std::dec << "[" << list[0];
    for(uint16_t i = 1; i < entriesInList; ++i) {
        std::cout <<"] - [" << list[i];
    }
    std::cout << "]" << std::endl;
    return 0;
}
