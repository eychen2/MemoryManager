//
// Created by echen on 3/5/2022.
//
#include "MemoryManager.h"
#include <stdlib.h>
#include <iostream>
#include <utility>
#include <sstream>
#include <cmath>
uint8_t binaryToDec(std::string input)
{
    uint8_t dec;
    for(int i=input.size()-1; i>=0;i--)
    {
        dec*=2;
        dec+=input[i]-'0';
    }
    return dec;
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
    numWords=sizeInWords;
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
    return (char*)(start)+best*wordSize;
}
void MemoryManager::free(void* address)
{
    int offset=((int)((char*)(address)-(char*)(start)))/wordSize;
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
    int size = std::ceil((double)numWords/8)+2;
    uint8_t* bitmap = new uint8_t[size];
    std::string cheese;
    auto hIt = holes.begin();
    auto mIt = memory.begin();
    while(hIt!=holes.end()&&mIt!=memory.end())
    {
        int num=0;
        if(hIt->offset<mIt->offset)
        {
            for(unsigned int i=0; i<hIt->length;++i)
                cheese+='0';
            ++hIt;
        }
        else
        {
            for(unsigned int i=0; i<mIt->length;++i)
                cheese+='1';
            ++mIt;
        }
    }
    while(hIt!=holes.end())
    {
        for(unsigned int i=0; i<hIt->length;++i)
            cheese+='0';
        ++hIt;
    }
    while(mIt!=memory.end())
    {
        for(unsigned int i=0; i<mIt->length;++i)
            cheese+='1';
        ++mIt;
    }
    bitmap[0]= size-2;
    bitmap[1]= (size-2)>>8;
    int count=2;
    for(unsigned int i=0;i<numWords/8;++i)
    {
        std::string temp = cheese.substr(8*i,8);
        bitmap[count]= binaryToDec(temp);
        ++count;
    }
    std::string temp =  cheese.substr(8*(numWords/8),cheese.size()-(8*(numWords/8)));
    bitmap[count]= binaryToDec(temp);
    return bitmap;
}

unsigned int MemoryManager::getMemoryLimit()
{
    return wordSize*numWords;
}
void MemoryManager::setAllocator(std::function<int(int, void *)> allocator) {
    this->allocator=allocator;
}
int bestFit(int sizeInWords, void *list)
{
    int result = -1;
    int diff = INT16_MAX;
    uint16_t* holeList = static_cast<uint16_t*>(list);
    int size = *holeList++;
    for(unsigned int i=0;i<size;++i)
    {
        if(holeList[2*i+1]>=sizeInWords&&holeList[2*i+1]-sizeInWords<diff)
        {
            result=holeList[2*i];
            diff=holeList[2*i+1]-sizeInWords;
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
    for(unsigned int i=0;i<size;i++)
    {
        if(holeList[2*i+1]>=sizeInWords&&holeList[2*i+1]-sizeInWords>diff)
        {
            result=holeList[2*i];
            diff=holeList[2*i+1]-sizeInWords;
        }
    }
    return result;
}