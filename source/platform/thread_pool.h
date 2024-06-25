#pragma once

#include "source/core/types.h"

struct work_queue;

#define WORK_QUEUE_CALLBACK(name) void name(work_queue *Queue, void *Data)
typedef WORK_QUEUE_CALLBACK(work_queue_callback);

struct work_queue_entry
{
    work_queue_callback *Callback;
    void *Data;
};

struct work_queue
{
    u32 volatile CompletionGoal;
    u32 volatile CompletionCount;
    
    u32 volatile NextEntryToWrite;
    u32 volatile NextEntryToRead;
    void *SemaphoreHandle;
    
    work_queue_entry Entries[256];
};

struct work_thread
{
    void *Handle;
    work_queue *Queue;
};
