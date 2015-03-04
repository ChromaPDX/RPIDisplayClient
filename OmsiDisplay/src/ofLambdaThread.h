//
//  ofLambdaThread.h
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/19/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#ifndef __OmsiDisplay__ofLambdaThread__
#define __OmsiDisplay__ofLambdaThread__

#include "ofThread.h"
#include "ofFileUtils.h"
#include <functional>
#include <queue>
#include "ofAppRunner.h"
#include <sys/syscall.h>

class BackgroundThread : public ofThread {
    
    queue<std::function<bool(void)>> operations;
    long tid = 0;
    
public:
    
    BackgroundThread() : ofThread() {
        thread.setPriority((Poco::Thread::Priority)Poco::Thread::getMinOSPriority());
        //printf("spawn thread priority (%d) \n", thread.getPriority());
    }
    
    void addOperation(std::function<bool(void)> operation){
        if (syscall(SYS_gettid) == tid){
            //printf("already on bg thread : %d", getThreadId())
            operations.push(operation);
            return;
        }
        printf("background dispatch %ld -> %ld \n", syscall(SYS_gettid), tid);
        lock();
        operations.push(operation);
        unlock();
    }
    
    void threadedFunction() {
        
        // start
        
        while(isThreadRunning()) {
            
            if (!tid) tid = syscall(SYS_gettid);
            
            lock();
            if (operations.size()){
                std::function<bool(void)> proc = operations.front();
                unlock();
                if (proc()){
                    lock();
                    operations.pop();
                    unlock();
                }
            }
            else {
                unlock();
            }
            
            
            ofSleepMillis(100);
        }
        
        // done
    }
};


class LThread : public ofThread {
    
public:
    
    LThread() : ofThread() {
        thread.setPriority((Poco::Thread::Priority)Poco::Thread::getMinOSPriority());
        printf("spawn thread priority (%d) \n", thread.getPriority());
    }
    
    LThread(function<bool(void)> operationBlock_) : LThread() {
        operationBlock = operationBlock_;
    }
    
    function<bool(void)> operationBlock;
    
    void threadedFunction() {
        
        // start
        
        while(isThreadRunning()) {
            if (operationBlock()){
                operationBlock = nullptr;
                stopThread();
            }
            
            ofSleepMillis(100);
        }
        
        // done
    }
};





#endif /* defined(__OmsiDisplay__ofLambdaThread__) */
