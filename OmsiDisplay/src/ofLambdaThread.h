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



template <typename I> class TLThread : public ofThread {
    
public:
    
    I input;
    function<bool(I)> operationBlock;
    
    void threadedFunction() {
        
        // start
        
        while(isThreadRunning()) {
            if (operationBlock(input)) stopThread();
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
        }
        
        // done
    }
};





#endif /* defined(__OmsiDisplay__ofLambdaThread__) */
