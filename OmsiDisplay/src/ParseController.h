//
//  ParseController.h
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/16/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#ifndef __OmsiDisplay__ParseController__
#define __OmsiDisplay__ParseController__

#include "ofxJSON.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <string>

using namespace std;

class ViewModel;

class ParseController {
    
    shared_ptr<Json::Value> apiCall(string requestType, string urlSuffix, shared_ptr<Json::Value> json, int contentType = 0);
    shared_ptr<Json::Value> apiCall(string requestType, ofFile& file, int contentType = 1);
    
public:
    
    shared_ptr<Json::Value> request;
    shared_ptr<Json::Value> result;
    
    static const string appId;
    static const string clientId;
    static const string restKey;
    
    static const string parseDomain;
    static const string objectPrefix;
    static const string filePrefix;
    static const string functionPrefix;
    
    void testCurl();
    
    shared_ptr<Json::Value> fetchAll(string className);
    shared_ptr<Json::Value> callFunction(string functionName, shared_ptr<Json::Value>);
    shared_ptr<Json::Value> updateObject(shared_ptr<ViewModel> object);
    shared_ptr<Json::Value> uploadFile(ofFile& file);
    
    shared_ptr<Json::Value> createNewDisplay();
    
    shared_ptr<Json::Value> post();
    shared_ptr<Json::Value> get();
    
    string getMacAddress();
};

#endif /* defined(__OmsiDisplay__ParseController__) */
