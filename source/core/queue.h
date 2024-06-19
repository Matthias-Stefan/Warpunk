#pragma once

#include <optional>

template<typename T, size_t S>
struct static_queue
{
    T E[S];
    void Enqueue(T Element);
    std::optional<T> Dequeue();
    
    index Head;
    index Tail;
    size_t Used = 0;
    const size_t Size = S;
};

template<typename T, size_t S>
void 
static_queue<T, S>::Enqueue(T Element) 
{
    if (Used == S)
    {
        return;
    }
    
    index Next = (Tail + 1) % Size;
    E[Next] = Element;
    Tail = Next;
    Used++;
}

template<typename T, size_t S>
std::optional<T>
static_queue<T, S>::Dequeue() 
{
    if (Used == 0)
    {
        return std::nullopt;
    }
    
    T Result = E[Head];
    Head = (Head++) % Size;
    Used--;
    return Result;
}

