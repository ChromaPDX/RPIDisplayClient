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
#include <sys/syscall.h>

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
    
    queue<std::function<bool(void)>> operations;
    queue<shared_ptr<Asset>> downloadQueue;
    bool _downloading {false};
    ofURLFileLoader loader;
    function<void(void)> completionBlock;
    long tid = 0;
    
public:
    
    bool downloading(){
        return _downloading;
    }
    
    void addOperation(std::function<bool(void)> operation){
        if (syscall(SYS_gettid) == tid){
            operations.push(operation);
            return;
        }
        printf("web dispatch %ld -> %ld \n", syscall(SYS_gettid), tid);
        lock();
        operations.push(operation);
        unlock();
    }
    
    void queueDownload(shared_ptr<Asset> asset){
        if (syscall(SYS_gettid) == tid){
            //printf("on web thread, queue download %d : %s \n", downloadQueue.size(), asset->url.c_str());
            // already on web thread, don't need to lock mutex
            downloadQueue.push(asset);
            return;
        }
        printf("web dispatch %ld -> %ld \n", syscall(SYS_gettid), tid);
        lock();
        downloadQueue.push(asset);
        printf("queue download %d : %s \n", downloadQueue.size(), asset->url.c_str());
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
            
            if (!_downloading){
                
                if (downloadQueue.size()){
                    
                    _downloading = true;
                    
                    ofRegisterURLNotification(this);
                    
                    lock();
                    auto asset = downloadQueue.front();
                    unlock();
                    
                    string path = ofToDataPath("./assets/", false) + "dl_" + asset->md5name();
                    
                    ofFile existing(path);
                    
                    if (existing.exists()){
                        cout << "Erase partial download" << endl;
                        existing.remove();
                    }
                    
                    cout << "Begin download of : " << asset->url << " to " << path << endl;
                    
                    loader.saveAsync(asset->url, path);
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
    
    void fetchData();
    void softwareUpdate();
    
    bool savePlaylist(shared_ptr<Json::Value> playlistItems);
    shared_ptr<Json::Value> loadCachedPlaylist();
    
    string deviceId;
    shared_ptr<DisplayClient> me;
    
    WebThread webThread; // BACKGROUND THREAD
    
    BackgroundThread bgThread;
    BackgroundThread parseThread;
    
    static queue<std::function<void(void)>> mainQueue;
    static std::mutex mainLock;
    static long mainQueueId;
    
    static void runOnMainQueue(std::function<void(void)> operation);
    shared_ptr<Playlist> defaultPlaylist();
    
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
    void setItems(shared_ptr<Json::Value> playlistItems, bool isOnline);
    
    void setup();
    void update();
    void draw();
    
    void uploadScreenShots();
    void takeScreenShot(int frame);
    
    unsigned int counter = 0;
};

#endif /* defined(__OmsiDisplay__PlaylistController__) */
