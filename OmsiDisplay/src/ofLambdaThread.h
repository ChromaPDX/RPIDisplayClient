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

class BackgroundThread : public ofThread {
    
public:
    
    queue<std::function<bool(void)>> operations;
    
    BackgroundThread() : ofThread() {
        thread.setPriority((Poco::Thread::Priority)Poco::Thread::getMinOSPriority());
        printf("spawn thread priority (%d) \n", thread.getPriority());
    }
    
    void addOperation(std::function<bool(void)> operation){
        lock();
        operations.push(operation);
        unlock();
    }
    
    void threadedFunction() {
        
        // start
        
        while(isThreadRunning()) {
            
            if (operations.size()){
                if (operations.front()()){
                    lock();
                    operations.pop();
                    unlock();
                }
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
