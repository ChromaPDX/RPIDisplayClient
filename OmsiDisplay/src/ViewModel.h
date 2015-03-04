//
//  Asset.h
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/27/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//
    
#ifndef __OmsiDisplay__ViewModel__
#define __OmsiDisplay__ViewModel__

#include "ofMain.h"
#include "ofxJSON.h"

#include <chrono>

using namespace Json;

typedef std::chrono::system_clock::time_point timePoint;

timePoint string_to_time_point(const std::string &str);
std::string time_point_to_string(timePoint &tp);
std::string current_time_to_string();
Value time_point_to_json(timePoint &tp);

typedef enum AssetType {
    AssetTypeNone,
    AssetTypePhoto,
    AssetTypeVideo,
} AssetType;

class ViewModel {
    
public:
    ViewModel(){};
    ~ViewModel(){};
    
    string objectId;
    timePoint createdAt;
    timePoint updatedAt;

    virtual string className() = 0;
    
    virtual shared_ptr<Value> toJson() {
        auto out = make_shared<Value>();
        (*out)["objectId"] = objectId;
        (*out)["createdAt"] = time_point_to_string(createdAt);
        (*out)["updatedAt"] = time_point_to_string(updatedAt);
        return out;
    }
    
    virtual void readJson(const Value& json){
        validateString(json, "objectId", objectId);
        validateDate(json, "createdAt", createdAt);
        validateDate(json, "updatedAt", updatedAt);
    }
    
    virtual const Value updatedKeys() {
        return Value();
    }
    
    void validateString(const Value& json, const string& key, string& out){
        if (!json[key].empty()){
            //printf("set %s for key %s \n", json[key].asString().c_str(), key.c_str());
            out = json[key].asString();
        }
    }
    
    void validateNumber(const Value& json, const string& key, int& out){
        if (!json[key].empty()){
            //printf("set %s for key %s \n", json[key].asString().c_str(), key.c_str());
            out = json[key].asInt();
        }
    }
    
    void validateBool(const Value& json, const string& key, bool& out){
        if (!json[key].empty()){
            //printf("set %s for key %s \n", json[key].asString().c_str(), key.c_str());
            out = json[key].asBool();
        }
    }
    
    void validateDate(const Value& json, const string& key, timePoint& out){
        if (!json[key].empty()){
            if (key == "createdAt" || key == "updatedAt"){
                string dateString = json[key].asString();
                out = string_to_time_point(dateString);
                //std::time_t epoch_time = std::chrono::system_clock::to_time_t(out);
                //printf("time %s -> %ld -> %s\n", dateString.c_str(), epoch_time, time_point_to_string(out).c_str());
            }
            else if (!json[key]["iso"].empty()){ // DATE OBJECT WITH TYPE
                string dateString = json[key]["iso"].asString();
                out = string_to_time_point(dateString);
                //std::time_t epoch_time = std::chrono::system_clock::to_time_t(out);
                //printf("time %s -> %ld -> %s\n", dateString.c_str(), epoch_time, time_point_to_string(out).c_str());
            }
        }
    }
    
    ViewModel(const Value& json){
        readJson(json);
    };
    
    virtual void print(){printf("native %s : %s \n", className().c_str(), toJson()->toStyledString().c_str());};
    
private:
    
};

class Asset : public ViewModel {
    
public:
    using ViewModel::ViewModel;
    
    // PERSISTENT
    AssetType type;
    string url;
    
    string checksum;
    
    timePoint addDate;
    timePoint removeDate;
    
    bool enabled {1};
    
    // CONVENIENCE
    string localPath;
    bool isAvailable {false};
    string md5name();
    string name();
    string extension();
    
    function<void(void)> completionBlock;
    float version {0.0};
    
    static shared_ptr<Asset> fromJson(const Value& json);
    
    string className() override {return "Asset";};
    
    Asset(const Value& json) {
        readJson(json);
    }
    
    Asset() {
    }
    
    void readJson(const Value& json) override{
        using namespace std::chrono;
        ViewModel::readJson(json);
        
        validateString(json, "url", url);
        validateString(json, "checksum", checksum);
        validateDate(json, "addDate", addDate);
        validateDate(json, "removeDate", removeDate);
        validateBool(json, "enabled", enabled);
        
        if (!json["assetType"].empty()){
            type = (AssetType)json["assetType"].asInt();
        };
    }
    shared_ptr<Value> toJson() override {
        auto out = ViewModel::toJson();
        (*out)["url"] = url;
        (*out)["type"] = type;
        return out;
    }
};

class PlaylistItem : public ViewModel {
    
public:
    using ViewModel::ViewModel;
    
    shared_ptr<Asset> asset;
    int duration {0};
    int index {0};
    
    static shared_ptr<PlaylistItem> fromJson(ofxJSONElement& json);
    
    string className() override {return "PlaylistItem";};
    
    PlaylistItem(AssetType type, string file_, int duration_){
        asset = make_shared<Asset>();
        asset->type = type;
        asset->isAvailable = true;
        asset->localPath = file_;
        duration = duration_;
    }
    
    PlaylistItem(const Value& json) {
        readJson(json);
    }
    
    void readJson(const Value& json) override{
        ViewModel::readJson(json);
        
        //printf("PlaylistItem::readJson \n");
        
        validateNumber(json, "duration", duration);
        validateNumber(json, "index", index);

        if (!json["asset"].empty()){
            Value jdefault;
            asset = Asset::fromJson(json.get("asset", jdefault));
        };
    }
    
    shared_ptr<Value> toJson() override {
        auto out = ViewModel::toJson();
        (*out)["duration"] = duration;
        (*out)["asset"] = *asset->toJson();
        return out;
    }
    
    static shared_ptr<PlaylistItem> fromJson(const Value& json);
    static shared_ptr<PlaylistItem> fromData(AssetType type_, string file_, int duration = 0);
};

class Playlist : public ViewModel {
    
public:
    
    vector<shared_ptr<PlaylistItem>> items;
    
    string className() override {return "ViewModel";};
    
    static shared_ptr<Playlist> fromJson(Value& json);
 
    Playlist(){};
    ~Playlist(){};
    
    
};

class DisplayClient : public ViewModel {
    
public:
    string deviceId;
    string zone;
    string callCustomFunction;
    
    // META
    int loopsSinceBoot = 0;
    int failedFetchRequests = 0;
    string appVersion;
    
    Json::Value screenShot;
    Json::Value screenShots;
    
    static shared_ptr<DisplayClient> fromJson(const Value& json);
    
    string className() override {return "Display";};
    
    shared_ptr<Value> toJson() override {
        auto out = ViewModel::toJson();
        (*out)["deviceId"] = deviceId;
        (*out)["zone"] = zone;
        (*out)["callCustomFunction"] = callCustomFunction;
        // META-DATA
        (*out)["loopsSinceBoot"] = loopsSinceBoot;
        (*out)["upTime"] = ofGetElapsedTimef();
        (*out)["failedFetchRequests"] = failedFetchRequests;
        if (!screenShot.empty()) (*out)["screenShot"] = screenShot;
        if (!screenShots.empty()) (*out)["screenShots"] = screenShots;
        return out;
    }
    
    DisplayClient(const Value& json) {
        readJson(json);
    }
    
    void readJson(const Value& json) override{
        ViewModel::readJson(json);
        validateString(json, "deviceId", deviceId);
        validateString(json, "zone", zone);
        validateString(json, "callCustomFunction", callCustomFunction);
        // loopsSinceBoot is write only
        // uptime is write only
        
        if (!json["screenShot"].empty()) {
            screenShot = json["screenShot"];
            screenShot["__type"] = "File";
        }
        if (!json["screenShots"].empty()) {
            screenShots = json["screenShots"];
        }
    }
    
    const Value updatedKeys() override {
        auto out = Value();
        (out)["deviceId"] = deviceId;
        (out)["zone"] = zone;
        (out)["callCustomFunction"] = callCustomFunction;
        // META-DATA
        (out)["loopsSinceBoot"] = loopsSinceBoot;
        (out)["upTime"] = ofGetElapsedTimef();
        (out)["failedFetchRequests"] = failedFetchRequests;
        (out)["appVersion"] = appVersion;
        // DATE
        auto date = Value();
        date["__type"] = "Date";
        date["iso"] = current_time_to_string();

        (out)["lastCheckIn"] = date;
        
        if (!screenShot.empty()) (out)["screenShot"] = screenShot;
        if (!screenShots.empty()) (out)["screenShots"] = screenShots;
        
        //printf("updated keys %s \n", out.toStyledString().c_str());
        
        return out;
    }
    
};


#endif /* defined(__OmsiDisplay__Asset__) */


//my items : {
//    "result" : [
//                {
//                    "__type" : "Object",
//                    "addDate" : {
//                        "__type" : "Date",
//                        "iso" : "2015-02-05T20:52:00.000Z"
//                    },
//                    "asset" : {
//                        "__type" : "Object",
//                        "className" : "Asset",
//                        "createdAt" : "2015-02-05T20:49:40.535Z",
//                        "objectId" : "1TDrhhDc5S",
//                        "updatedAt" : "2015-02-05T20:50:00.037Z",
//                        "url" : "https://www.dropbox.com/s/ywr7ephexo6fl63/omniMax.jpg?dl=1"
//                    },
//                    "assetURL" : "https://www.dropbox.com/s/ywr7ephexo6fl63/omniMax.jpg?dl=1",
//                    "className" : "PlaylistItem",
//                    "createdAt" : "2015-02-05T20:52:06.619Z",
//                    "duration" : 10,
//                    "objectId" : "HJRBp64Rzq",
//                    "playlist" : {
//                        "__type" : "Pointer",
//                        "className" : "Playlist",
//                        "objectId" : "hQId3Pvbo1"
//                    },
//                    "removeDate" : {
//                        "__type" : "Date",
//                        "iso" : "2015-03-05T20:52:00.000Z"
//                    },
//                    "updatedAt" : "2015-02-05T23:39:40.280Z"
//                },
//                {
//                    "__type" : "Object",
//                    "addDate" : {
//                        "__type" : "Date",
//                        "iso" : "2015-02-05T20:51:00.000Z"
//                    },
//                    "asset" : {
//                        "__type" : "Object",
//                        "className" : "Asset",
//                        "createdAt" : "2015-02-05T20:50:37.901Z",
//                        "objectId" : "08tZYwiZyX",
//                        "updatedAt" : "2015-02-05T20:50:37.901Z",
//                        "url" : "https://www.dropbox.com/s/pfh9cmhcw1anvuo/turbine.jpg?dl=1"
//                    },
//                    "assetURL" : "https://www.dropbox.com/s/pfh9cmhcw1anvuo/turbine.jpg?dl=1",
//                    "className" : "PlaylistItem",
//                    "createdAt" : "2015-02-05T20:51:18.532Z",
//                    "duration" : 20,
//                    "objectId" : "evKMy9VNuK",
//                    "playlist" : {
//                        "__type" : "Pointer",
//                        "className" : "Playlist",
//                        "objectId" : "hQId3Pvbo1"
//                    },
//                    "removeDate" : {
//                        "__type" : "Date",
//                        "iso" : "2015-03-05T20:51:00.000Z"
//                    },
//                    "updatedAt" : "2015-02-05T23:39:49.289Z"
//                }
//                ]
//}

