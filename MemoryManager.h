//
// Created by echen on 3/5/2022.
//
#include <unistd.h>
#include <functional>
#include <string>
#include <list>
#ifndef MEMORYMANAGER_MEMORYMANAGER_H
#define MEMORYMANAGER_MEMORYMANAGER_H
struct chunk
{
    int offset;
    int length;
    chunk(int offset_,int length_);
};

class MemoryManager
{
private:
    int numWords=0;
    unsigned wordSize=0;
    std::function<int(int, void *)> allocator;
    void* start;
    std::allocator<char*> allocatedmem;
    std::list<chunk> memory;
    std::list<chunk> holes;

public:
    MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator);
    ~MemoryManager();
    void initialize(size_t sizeInWords);
    void shutdown();
    void *allocate(size_t sizeInBytes);
    void free(void *address);
    void setAllocator(std::function<int(int, void *)> allocator);
    int dumpMemoryMap(char *filename);
    void *getList();
    void *getBitmap();
    unsigned getWordSize();
    void *getMemoryStart();
    unsigned getMemoryLimit();

};
int bestFit(int sizeInWords, void *list);
int worstFit(int sizeInWords, void *list);
uint8_t binaryToDec(std::string input);
#endif //MEMORYMANAGER_MEMORYMANAGER_H
