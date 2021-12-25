#pragma once
#include <queue>
#include <mutex>
#include "ItemInfo.hpp"

class ItemQueue
{
    private:
    static std::queue<cItemInfo> q;
    static std::mutex mtx;
	size_t BufCapacity = 20;
    ItemQueue(){};
    ~ItemQueue(){};
    public:
    static void AddItem(const cItemInfo& pItem);
    static void PopItem();
    static cItemInfo& FrontItem();
    static size_t GetQueueSize();
    static void EmptyQueue();
};