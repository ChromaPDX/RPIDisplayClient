//
//  PlaylistController.h
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/27/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#ifndef __OmsiDisplay__PlaylistController__
#define __OmsiDisplay__PlaylistController__

#include <functional>

#include "ofLambdaThread.h"
#include "ofFileUtils.h"
#include "ViewModel.h"
#include "ofImage.h"
#include "ofxOMXPlayer.h"
#include "ParseController.h"
#include "URLImage.h"
#include <mutex>

class PlaylistItemPlayer {
    
    shared_ptr<PlaylistItem> _playlistItem = nullptr;
    
public:
    
    // VIDEO
    
    static ofxOMXPlayer *omxPlayer;
    ofxOMXPlayerSettings settings;
    
    ofxOMXPlayerListener* listener;
    
    bool loadMovie(shared_ptr<PlaylistItem> item);
    
    // STILL
    
    shared_ptr<ofImage> image;
    int alpha {0};
    
    void draw();
    void clear();
    
    shared_ptr<PlaylistItem> playlistItem(){return _playlistItem;};
    
    void loadPlaylistItem(shared_ptr<PlaylistItem> item);
    bool beginTransitionToAsset(shared_ptr<PlaylistItem> item);
    bool transitionToAsset(shared_ptr<PlaylistItem> item);
    void finishTransitionToAsset(shared_ptr<PlaylistItem> item);
    
    unsigned long long skipTimeStart=0;
    unsigned long long skipTimeEnd=0;
    unsigned long long amountSkipped =0;
    unsigned long long totalAmountSkipped =0;
};

typedef enum TransitionState {
    TransitionLoad,
    TransitionTransition,
    TransitionUnload,
    TransitionPlay,
    TransitionWaiting,
} TransitionState;

class Playlist;

class WebThread : public ofThread {
    
public:
    
    bool downloading {false};
    
    queue<std::function<bool(WebThread*)>> operationQueue;
    vector<shared_ptr<Asset>> downloadQueue;
    
    ofURLFileLoader loader;
    
    void threadedFunction() {
        
        // start
        
        while(isThreadRunning()) {
            
            if (!downloading){
                
                if (operationQueue.size()){
                    
                    if (operationQueue.front()(this)){
                        lock();
                        operationQueue.pop();
                        unlock();
                    }
                    
                }
                
                if (downloadQueue.size()){
                    downloading = true;
                    ofRegisterURLNotification(this);
                    auto asset = downloadQueue.front();
                    
                    
                    string path = ofToDataPath("./assets/", false) + "dl_" + asset->md5name();
                    
                    ofFile existing(path);
                    
                    if (existing.exists()){
                        cout << "Erase partial download" << endl;
                        existing.remove();
                    }
                    
                    cout << "Begin download of : " << asset->url << " to " << path << endl;
                    
                    loader.saveAsync(asset->url, path);
                    
                    //                    loader.saveTo(asset->url, path);
                    //
                    //                    cout << "Finish download of : " << asset->name() << endl;
                    //                    downloadQueue.front()->isAvailable = true;
                    //                    downloadQueue.erase(downloadQueue.begin());
                    //                    downloading = false;
                    //                    ofUnregisterURLNotification(this);
                }
            }
            
            ofSleepMillis(100);
            
        }
        
        // done
    }
    
    void urlResponse(ofHttpResponse& response);
    
};

class PlaylistController : public ofxOMXPlayerListener {
    
    // TRACK META-DATA
    int loopsSinceBoot = 0;
    int failedFetchRequests = 0;
    
public:
    
    ParseController parseController;
    
    bool fetchData();
    
    bool savePlaylist(shared_ptr<Json::Value> playlistItems);
    shared_ptr<Json::Value> loadCachedPlaylist();
    
    shared_ptr<DisplayClient> me;
    
    WebThread webThread; // BACKGROUND THREAD
    
    BackgroundThread bgThread;
    BackgroundThread parseThread;
    
    queue<std::function<void(void)>> mainQueue;
    std::mutex mainLock;
    
    void runOnMainQueue(std::function<void(void)> operation){
        mainLock.lock();
        mainQueue.push(operation);
        mainLock.unlock();
    }
    
    shared_ptr<Playlist> testPlaylist();
    shared_ptr<Playlist> playlist;
    vector<shared_ptr<PlaylistItem>> playlistItems;
    
    shared_ptr<PlaylistItem> currentItem;
    
    int currentPlayer = 0;
    
    PlaylistItemPlayer players[2];
    
    void onVideoEnd(ofxOMXPlayerListenerEventData& e);
    void onVideoLoop(ofxOMXPlayerListenerEventData& e){ /*empty*/ };
    
    //bool shouldGetNextItem {false};
    void getNextItem();
    
    TransitionState transitionState = TransitionLoad;
    
    shared_ptr<Json::Value> fetchItems();
    void setItems(shared_ptr<Json::Value> playlistItems);
    
    void setup();
    void update();
    void draw();
    
    void uploadScreenShots();
    void takeScreenShot(int frame);
    
    unsigned int counter = 0;
};

#endif /* defined(__OmsiDisplay__PlaylistController__) */
