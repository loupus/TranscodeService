#include "ItemQueue.hpp"

  std::queue<cItemInfo> ItemQueue::q;
  std::mutex ItemQueue::mtx;

void ItemQueue::AddItem(const cItemInfo& pItem ) 
{
    std::unique_lock<std::mutex> lock(mtx);
    q.push(pItem);
    lock.unlock();
}

void ItemQueue::PopItem() 
{
    std::unique_lock<std::mutex> lock(mtx);
    q.pop();
    lock.unlock();
}

cItemInfo& ItemQueue::FrontItem() 
{   
    std::unique_lock<std::mutex> lock(mtx);
    cItemInfo& back = q.front();
    lock.unlock();
    return back;
}

size_t ItemQueue::GetQueueSize() 
{
    std::unique_lock<std::mutex> lock(mtx);
    size_t back = 0;    
    back = q.size();
    lock.unlock();
    return back;
}

void ItemQueue::EmptyQueue() 
{
    std::unique_lock<std::mutex> lock(mtx);
    while (!q.empty())
    {
        q.pop();
    }
    lock.unlock();
}

ItemQueue* ItemQueue::GetInstance() 
{
    if (thisObj == nullptr)
    {
        thisObj = new ItemQueue();
    }
    return thisObj;
}

void ItemQueue::DeleteInstance() 
{
    if (thisObj != nullptr)
    {
        delete thisObj;
        thisObj = nullptr;
    }
}

ItemQueue::ItemQueue() 
{
    
}

ItemQueue::~ItemQueue() 
{
    EmptyQueue() ;
    DeleteInstance(); // ???
}
