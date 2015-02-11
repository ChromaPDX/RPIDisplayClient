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

using namespace Json;

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
    string createdAt;
    string updatedAt;
    
    virtual string className() = 0;
    
    virtual shared_ptr<Value> toJson() {
        auto out = make_shared<Value>();
        (*out)["objectId"] = objectId;
        (*out)["createdAt"] = createdAt;
        (*out)["updatedAt"]= updatedAt;
        return out;
    }
    
    virtual void readJson(const Value& json){
        validateString(json, "objectId", objectId);
        validateString(json, "createdAt", createdAt);
        validateString(json, "updatedAt", updatedAt);
    }
    
    virtual const Value updatedKeys() {
        return Value();
    }
    
    void validateString(const Value& json, const string& key, string& out){
        if (!json[key].empty()){
            printf("set %s for key %s \n", json[key].asString().c_str(), key.c_str());
            out = json[key].asString();
        }
    }
    
    void validateNumber(const Value& json, const string& key, int& out){
        if (!json[key].empty()){
            printf("set %s for key %s \n", json[key].asString().c_str(), key.c_str());
            out = json[key].asInt();
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
    string localPath;
    
    // CONVENIENCE
    bool isAvailable {false};
    string md5name();
    string name();
    
    static shared_ptr<Asset> fromJson(const Value& json);
    
    string className() override {return "Asset";};
    
    Asset(const Value& json) {
        readJson(json);
    }
    
    Asset() {
    }
    
    void readJson(const Value& json) override{
        ViewModel::readJson(json);
        validateString(json, "url", url);
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
    string addDate;
    string removeDate;
    int duration = {0};
    int index;
    
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
        
        printf("PlaylistItem::readJson \n");
        
        validateNumber(json, "duration", duration);
        validateNumber(json, "index", index);
        
//        if (!json["duration"].empty()){
//            duration = json["duration"].asInt();
//        };
//        if (!json["addDate"].empty()){
//            addDate = json["addDate"]["iso"].asString();
//        };
//        if (!json["removeDate"].empty()){
//            addDate = json["removeDate"]["iso"].asString();
//        };
        if (!json["asset"].empty()){
            Value jdefault;
            asset = Asset::fromJson(json.get("asset", jdefault));
        };
    }
    
    shared_ptr<Value> toJson() override {
        auto out = ViewModel::toJson();
        (*out)["duration"] = duration;
        (*out)["asset"] = *asset->toJson();
        //(*out)["url"] = asset->url;
        //(*out)["type"] = asset->assetType;
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
    Json::Value screenShot;
    Json::Value screenShots;
    
    static shared_ptr<DisplayClient> fromJson(const Value& json);
    
    string className() override {return "Display";};
    
    shared_ptr<Value> toJson() override {
        auto out = ViewModel::toJson();
        (*out)["deviceId"] = deviceId;
        (*out)["zone"] = zone;
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
        if (!screenShot.empty()) (out)["screenShot"] = screenShot;
        if (!screenShots.empty()) (out)["screenShots"] = screenShots;
        printf("updated keys %s \n", out.toStyledString().c_str());
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

