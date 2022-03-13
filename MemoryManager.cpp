//
// Created by echen on 3/5/2022.
//
#include "MemoryManager.h"
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <sstream>
#include <cmath>

std::string toHex(int input)
{
    std::stringstream sstream;
    sstream << std::hex << input;
    std::string result = sstream.str();
    return result;

}
chunk::chunk(int offset_, int length_)
{
    offset=offset_;
    length=length_;
}
MemoryManager::MemoryManager(unsigned wordSize, std::function<int(int, void *)> allocator)
{
    this->wordSize = wordSize;
    this->allocator = std::move(allocator);
    start=nullptr;
}
MemoryManager::~MemoryManager() {
    shutdown();
}
void MemoryManager::initialize(size_t sizeInWords)
{
    shutdown();
    start=malloc(sizeInWords*wordSize);
    holes.emplace_back(chunk(0,sizeInWords));
}

void MemoryManager::shutdown()
{
    if(start)
        std::free(start);
    start=nullptr;
    holes.clear();
    memory.clear();
}

void *MemoryManager::allocate(size_t sizeInBytes)
{
    uint16_t* list = static_cast<uint16_t*>(getList());
    int best = allocator(std::ceil((double)sizeInBytes/wordSize),list);
    delete[] list;
    if(best==-1)
        return nullptr;
    for(auto it=holes.begin();it!=holes.end();++it)
    {
        if(it->offset==best)
        {
            it->offset = it->offset + std::ceil((double)(sizeInBytes)/wordSize);
            it->length=it->length-std::ceil((double)(sizeInBytes)/wordSize);
            if(it->length==0)
                holes.erase(it);
            break;
        }
    }
    bool insert = false;
    for(auto it=memory.begin();it!=memory.end();++it)
    {
        if(best<it->offset)
        {
            memory.insert(it,chunk(best,std::ceil((double)(sizeInBytes)/wordSize)));
            insert=true;
            break;
        }
    }
    if(!insert)
        memory.emplace_back(chunk(best,std::ceil((double)(sizeInBytes)/wordSize)));
    return (char*)(start)+best;
}
void MemoryManager::free(void* address)
{
    int offset=(int)((char*)(address)-(char*)(start));
    int length=-1;
    for(auto it=memory.begin();it!=memory.end();++it)
    {
        if(offset==it->offset)
        {
            length=it->length;
            memory.erase(it);
            break;
        }
    }
    if(length==-1)
        return;
    bool merged=false;
    for(auto it =holes.begin();it!=holes.end();++it)
    {
        if(it->offset+it->length==offset)
        {
            merged=true;
            it->length+=length;
        }
        if(it->offset==offset+length&&merged)
        {
            int temp=it->length;
            it--;
            it->length+=temp;
            ++it;
            holes.erase(it);
            return;
        }
        if(it->offset==offset+length&&!merged)
        {
            it->offset=offset;
            it->length+=length;
            return;
        }
        if(it->offset>offset+length&&!merged)
            holes.insert(it,chunk(offset,length));
    }
}
int MemoryManager::dumpMemoryMap(char *filename)
{

}
unsigned MemoryManager::getWordSize()
{
    return wordSize;
}
void* MemoryManager::getMemoryStart()
{
    return start;
}

void* MemoryManager::getList()
{
    uint16_t* list = new uint16_t(2*holes.size()+1);
    int count=0;
    list[count++]=holes.size();
    for(auto it = holes.begin();it!=holes.end();++it)
    {
        list[count++]=it->offset;
        list[count++]=it->length;
    }
    return list;
    /*
    char list[4*holes.size()+1];
    int count=0;
    std::string temp= toHex(holes.size());
    list[count] = temp[0];
    ++count;
    list[count]=temp[1];
    ++count;
    for(auto it = holes.begin();it!=holes.end();++it)
    {
        temp = toHex(it->offset);
        list[count]=temp[0];
        ++count;
        list[count]=temp[1];
        ++count;
        temp=toHex(it->length);
        list[count]=temp[0];
        ++count;
        list[count]=temp[1];
        ++count;
    }
    return list;
     */
}

void* MemoryManager::getBitmap()
{

}

unsigned int MemoryManager::getMemoryLimit()
{
}
void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    this->allocator=allocator;
}
int bestFit(int sizeInWords, void *list)
{
    int result = -1;
    int diff = INT_MAX;
    uint16_t* holeList = static_cast<uint16_t*>(list);
    int size = *holeList++;
    for(unsigned int i=0;i<size;i+=2)
    {
        if(holeList[i+1]>=sizeInWords&&holeList[i+1]-sizeInWords<diff)
        {
            result=holeList[i];
            diff=holeList[i+1]-sizeInWords;
        }
    }
    return result;
}
int worstFit(int sizeInWords, void *list)
{
    int result = -1;
    int diff = -1;
    uint16_t* holeList = static_cast<uint16_t*>(list);
    int size = *holeList++;
    for(unsigned int i=0;i<size;i+=2)
    {
        if(holeList[i+1]>=sizeInWords&&holeList[i+1]-sizeInWords>diff)
        {
            result=holeList[i];
            diff=holeList[i+1]-sizeInWords;
        }
    }
    return result;
}