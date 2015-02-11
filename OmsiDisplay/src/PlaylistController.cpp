//
//  PlaylistController.cpp
//  OmsiDisplay
//
//  Created by Leif Shackelford on 1/27/15.
//  Copyright (c) 2015 Tim Knapen. All rights reserved.
//

#include "PlaylistController.h"
#include "ViewModel.h"
#include <Poco/Environment.h>

shared_ptr<Playlist> PlaylistController::testPlaylist() {
    return make_shared<Playlist>();
}

#define LIVE 1

string assetPath(){
    return ofToDataPath("./assets/", false);
}

void PlaylistController::setup(){
     printf("setup playlist \n");
    webThread.startThread();
    webThread.lock();
    getNextItem();
    webThread.unlock();
}

void PlaylistController::fetchData(){
    
    printf("fetch data \n");
    
    Json::Value defaultValue;
    
    shared_ptr<Json::Value> response(parseController.fetchAll("Display"));
    
    if (response){
        
        auto results = response->get("results", defaultValue);
        
        vector<shared_ptr<DisplayClient>>displays;
        
        auto deviceId = Poco::Environment::nodeId();
        
        for (auto& value : results){
            
            printf("fetched result: %s \n", value.toStyledString().c_str());
            
            if (value["deviceId"].asString() == deviceId){
                me = DisplayClient::fromJson(value);
            }
            
            displays.push_back(DisplayClient::fromJson(value));
        }
        
        if (!me) {
            parseController.createNewDisplay();
        }
        
        else {
            printf("update me : \n");
            
            uploadScreenShots();
            
            auto result = parseController.updateObject(me);
            
            if (result){
                me->readJson(*result);
                
                auto items = fetchItems();
                if (items) setItems(items);
                
            }
        }
    }
    else {
        printf("no server response, check back later \n");
    }
}

shared_ptr<Json::Value> PlaylistController::fetchItems() {
    auto result = parseController.callFunction("itemsForDisplay", me->toJson());
    
    if (result) {
        printf("my items : %s \n", result->toStyledString().c_str());
        
    }
    else {
        printf("error fetching my items : \n");
    }
    
    return result;
}

void PlaylistController::setItems(shared_ptr<Json::Value> playlistItems) {
    // TEST
    playlist = testPlaylist();
    // END TEST
    
#if LIVE
    
    Json::Value defaultValue;
    
    auto results = playlistItems->get("result", defaultValue);
    
    if (results.size() && playlist->items.size()) {
        printf("clear items \n");
        playlist->items.clear();
    }
    
    for (auto& value : results){
        
        auto item = PlaylistItem::fromJson(value);
        
        item->print();
        
        playlist->items.push_back(item);
    }
    
    sort(playlist->items.begin(), playlist->items.end(), [](shared_ptr<PlaylistItem> a, shared_ptr<PlaylistItem> b) -> bool
         {
             return a->index < b->index;
         });
    
    
    ofDirectory assetDirectory(ofToDataPath("./assets", false));
    
    if (assetDirectory.exists())
    {
        assetDirectory.listDir();
        
        auto allFiles = assetDirectory.getFiles();
        
        for (auto item : playlist->items) {
            
            for (auto& file : allFiles){
                if (file.getFileName() == item->asset->md5name()){
                    item->asset->isAvailable = true;
                    item->asset->localPath = file.getAbsolutePath();
                    cout << "validated asset : " << item->asset->name() << " -> " << file.getAbsolutePath() << endl;
                }
            }
            
            if (!item->asset->isAvailable) {
                printf("queue download %d : %s \n", webThread.downloadQueue.size(), item->asset->url.c_str());
                webThread.downloadQueue.push_back(item->asset);
            }
            
        }
    }

#else
    
    printf("enumerate directory: %s \n", assetPath().c_str());
    
    ofDirectory currentVideoDirectory(assetPath());
    
    if (currentVideoDirectory.exists())
    {
        currentVideoDirectory.listDir();
        currentVideoDirectory.sort();
        
        auto allFiles = currentVideoDirectory.getFiles();
        
        //        for (auto& file : allFiles){
        //             printf("file: %s , ext %s \n", file.path().c_str(), file.getExtension().c_str());
        //        }
        
        //        sort(allFiles.begin(), allFiles.end(), [](ofFile& a, ofFile& b) -> bool
        //        {
        //            return a.getExtension() < b.getExtension();
        //        });
        
        for (auto& file : allFiles){
            if (file.getExtension() == "mov" || file.getExtension() == "mp4") {
                playlist->items.push_back(PlaylistItem::fromData(AssetTypeVideo, file.path()));
            }
            else if (file.getExtension() == "jpg" || file.getExtension() == "jpeg" ||
                     file.getExtension() == "png" || file.getExtension() == "tiff") {
                playlist->items.push_back(PlaylistItem::fromData(AssetTypePhoto, file.path(), 10));
            }
        }
        
        for (auto item : playlist->items){
            if (item->asset->type == AssetTypePhoto) printf("(*)photo : %s \n", item->asset->localPath.c_str());
            else if (item->asset->type == AssetTypeVideo) printf("(*)video : %s \n", item->asset->localPath.c_str());
        }
        
    }
    else {
        printf("can't find video directory : %s \n", currentVideoDirectory.path().c_str());
    }
    
    
    //    sort(playlist->items.begin(), playlist->items.end(),
    //         [](shared_ptr<PlaylistItem> a, shared_ptr<PlaylistItem> b) -> bool
    //         {
    //             return a->file > b->file;
    //         });
    //
    //    for (auto& item : playlist->items){
    //        if (item->type == AssetTypeVideo){
    //            printf("setup omx player %s \n", item->file.c_str());
    //            videoController.setup(item->file);
    //            return;
    //        }
    //    }
    
#endif
    
    for (auto& player : players){
        player.listener = this;
    }
    
    ofEnableAlphaBlending();
    
    printf("finished setup \n");
}

void PlaylistController::getNextItem(){
    if (counter == 0) {
        if (!webThread.downloading) {
            printf("clear playlist \n");
            //playlist->items.clear();
            currentItem.reset();
            fetchData();
        }
    }
    
    if (playlist->items.size() > counter){
        transitionState = TransitionLoad;
        currentItem = playlist->items[counter];
        counter++;
        if (counter >= playlist->items.size()) counter = 0;
    }
    else {
        currentItem = nullptr;
        printf("error out of bounds playlist item request \n");
    }
    
    if (!currentItem->asset->isAvailable) {
        printf("asset : %s is unavailable / downloading", currentItem->asset->name().c_str());
        
        currentItem.reset();
        int availableCount = 0;
        for (auto item : playlist->items) {
            if (item->asset->isAvailable) availableCount++;
        }
        if (availableCount < 2) {
            printf("(%d of %d) wait to start til at least two assets are downloaded \n", availableCount, playlist->items.size());
            slideThread.stopThread();
            slideThread.operationBlock = [this]{
                sleep(10);
                int availableCount = 0;
                for (auto item : playlist->items) {
                    if (item->asset->isAvailable) availableCount++;
                }
                if (availableCount >= 2) {
                    mainQueue.push([this](){getNextItem();});
                    //shouldGetNextItem = true;
                    return true;
                }
                else {
                    printf("(%d of %d) wait to start til at least two assets are downloaded \n", availableCount, playlist->items.size());
                }
                return false;
            };
            slideThread.startThread();
            return;
        }
        getNextItem();
    }
}

void PlaylistController::update() { // MAIN THREAD
    
    if (mainQueue.size()){
        mainQueue.front()();
        mainQueue.pop();
    }
    
    bool hasAsset = false;
    
    if (currentItem){
        
        switch (transitionState){
            case TransitionLoad:
            
            for (auto& player : players){
                if (player.playlistItem()) hasAsset = true;
                if (player.beginTransitionToAsset(currentItem)){
                    players[1-currentPlayer].loadPlaylistItem(currentItem);
                    transitionState = TransitionTransition;
                    printf("start transition %d -> %d \n", currentPlayer, 1-currentPlayer);
                }
            }
            
            if (!hasAsset){ // edge case no players started yet
                players[1-currentPlayer].loadPlaylistItem(currentItem);
                transitionState = TransitionTransition;
            }
            
            break;
            
            case TransitionTransition:
            for (auto& player : players){
                if (player.transitionToAsset(currentItem)){
                    printf("finish transition to %s \n", player.playlistItem()->asset->name().c_str());
                    transitionState = TransitionUnload;
                }
            }
            break;
            
            case TransitionUnload:
            
            for (auto& player : players){
                player.finishTransitionToAsset(currentItem);
            }
            currentPlayer = 1 - currentPlayer;
            
            if (currentItem->duration != 0){
                slideThread.operationBlock = [this]{
                    sleep(currentItem->duration / 2);
                    mainQueue.push([this](){takeScreenShot(currentItem->index);});
                    sleep(currentItem->duration / 2);
                    
                    mainQueue.push([this](){getNextItem();});
                    //shouldGetNextItem = true;
                    return true;
                };
                slideThread.startThread();
            }
            else {
                slideThread.operationBlock = [this]{
                    sleep(5);
                    mainQueue.push([this](){takeScreenShot(currentItem->index);});
                    return true;
                };
                slideThread.startThread();
            }
            
            transitionState = TransitionPlay;
            break;
            default:
            break;
        }
        
    }
}

void PlaylistController::draw() {
    for (auto& player : players){
        player.draw();
    }
}

void PlaylistController::onVideoEnd(ofxOMXPlayerListenerEventData& e)
{
    ofLogVerbose(__func__) << " RECEIVED";
    mainQueue.push([this](){getNextItem();});
    //shouldGetNextItem = true;
}

void PlaylistController::uploadScreenShots(){
    
    if (playlist){
        int numImages = playlist->items.size();
        
        for (int frame = 0; frame < numImages; frame++) {
            string pad = "";
            if (frame < 100) {
                pad = "0";
                if (frame < 10) pad = "00";
            }
            char fileName[32];
            
            sprintf(fileName, "screenShot%s%d.jpg", pad.c_str(), frame);
            
            string path = assetPath() + string(fileName);
            ofFile screenShot(path);
            
            if (screenShot.exists()) {
                
                auto result = parseController.uploadFile(screenShot);
                
                if (result) {
                    
                    if (frame == 0){
                        me->screenShot = *result;
                        me->screenShot["__type"] = "File";
                    }
                    
                    me->screenShots[frame] = *result;
                    me->screenShots[frame]["__type"] = "File";
                }
            }
        }
    }
    
}

void PlaylistController::takeScreenShot(int frame) {
    
    shared_ptr<ofImage> screenShot = make_shared<ofImage>();
    
    screenShot->grabScreen(ofGetWidth() / 4, ofGetHeight() / 4, ofGetWidth() / 2, ofGetHeight() / 2);
    
    saveThread.operationBlock = [screenShot, frame](){
        
        screenShot->resize(ofGetWidth() / 4, ofGetHeight() / 4);
        
        string pad = "";
        if (frame < 100) {
            pad = "0";
            if (frame < 10) pad = "00";
        }
        
        char fileName[32];
        sprintf(fileName, "screenShot%s%d.jpg", pad.c_str(), frame);

        string path = assetPath() + string(fileName);
        
        cout << "grab screen shot : " << fileName << endl;
        
        ofFile file(path);
        
        if (file.exists()) {
            file.remove();
        }
        
        screenShot->saveImage(path, OF_IMAGE_QUALITY_MEDIUM);
        cout << "finish writing screenshot" << endl;
        
        return true;
    };
    
    saveThread.startThread();
}

//--------------------------------------------------------------
#pragma mark - PlaylistItemPlayer
//--------------------------------------------------------------

ofxOMXPlayer* PlaylistItemPlayer::omxPlayer = nullptr;

void PlaylistItemPlayer::loadPlaylistItem(shared_ptr<PlaylistItem> item){
    
    clear();
    
    _playlistItem = item;
    
    if (_playlistItem) {
        if (_playlistItem->asset->type == AssetTypePhoto){
            printf("load photo %s \n", _playlistItem->asset->localPath.c_str());
            image.loadImage(_playlistItem->asset->localPath);
        }
        else if (_playlistItem->asset->type == AssetTypeVideo){
            printf("load video %s \n", _playlistItem->asset->localPath.c_str());
            loadMovie(_playlistItem);
        }
    }
}

bool PlaylistItemPlayer::beginTransitionToAsset(shared_ptr<PlaylistItem> item) {
    // BEFORE NEXT ASSET IS LOADED
    if (!_playlistItem) return false;

    if (item == _playlistItem){
        return false;
        // prepare
    }
    else {
        if (_playlistItem->asset->type == AssetTypeVideo || item->asset->type == AssetTypeVideo) {
            alpha -=2;
            if (alpha <= 0)  {
                alpha = 0;
                if (_playlistItem->asset->type == AssetTypeVideo){
                    omxPlayer->setPaused(true);
                    //omxPlayer->getTextureReference().clear();
                    //omxPlayer->close();
                    //delete omxPlayer;
                    //omxPlayer = nullptr;
                    clear();
                }
                return true;
            }
            return false;
        }
        return true;
    }
}

bool PlaylistItemPlayer::transitionToAsset(shared_ptr<PlaylistItem> item) {
    // AFTER NEXT ASSET IS LOADED
    if (!_playlistItem) return false;
    
    if (item == _playlistItem){
        alpha +=3;
        //printf("fade in -> %d \n", alpha);
        if (alpha >= 255){
            alpha = 255;
            return true;
        }
        return false;
    }
    else {
        // player transion OUT
        //if (item->asset->type == AssetTypePhoto){
            alpha -=3;
            if (alpha <= 0)  {
                alpha = 0;
            }
        //}
        return false;
    }
}

void PlaylistItemPlayer::finishTransitionToAsset(shared_ptr<PlaylistItem> item) {
    // AFTER TRANSITION IS COMPLETED LOADED
    if (!_playlistItem) return;
    
    if (item == _playlistItem){
        // finish transition IN
    }
    else if (_playlistItem->asset->type == AssetTypePhoto){
        // finish transition OUT
        clear();
    }
}

void PlaylistItemPlayer::clear() {
    if (_playlistItem) {
        if (_playlistItem->asset->type == AssetTypeVideo) {
        }
        else if (_playlistItem->asset->type == AssetTypePhoto){
            image.clear();
        }
        
        _playlistItem.reset();
        alpha = 0;
    }
}


//--------------------------------------------------------------

void PlaylistItemPlayer::loadMovie(shared_ptr<PlaylistItem> item)
{
    if (!omxPlayer) omxPlayer = new ofxOMXPlayer();
    
    ofSetLogLevel("ofThread", OF_LOG_ERROR);
    printf("setup omx player");
    
    
    
    settings.videoPath = item->asset->localPath;
    settings.enableAudio	  = true;
    settings.useHDMIForAudio = true;	//default true
    settings.enableLooping = (item->duration != 0);		//default true
    settings.enableTexture = true;		//default true
    settings.listener = listener;
    
    omxPlayer->setup(settings);
    
    //    omxPlayer.loadMovie(path);
    //    skipTimeStart = ofGetElapsedTimeMillis();
    //    omxPlayer.loadMovie(path);
    //    skipTimeEnd = ofGetElapsedTimeMillis();
    //    amountSkipped = skipTimeEnd-skipTimeStart;
    //    totalAmountSkipped+=amountSkipped;
}

void PlaylistItemPlayer::draw(){
    if (_playlistItem){
        if (_playlistItem->asset->type == AssetTypePhoto){
            if (image.isAllocated()){
                ofSetColor(255, 255, 255, alpha);
                image.draw(0,0,ofGetWidth(), ofGetHeight());
            }
        }
        else if (_playlistItem->asset->type == AssetTypeVideo){
            if (omxPlayer){
                if (omxPlayer->isTextureEnabled()){
                    ofSetColor(255, 255, 255, alpha);
                    omxPlayer->draw(0, 0, ofGetWidth(), ofGetHeight());
                    
                    //                    stringstream info;
                    //                    info <<"\n" << _playlistItem->file;
                    //                    info <<"\n" <<	"MILLIS SKIPPED: "		<< amountSkipped;
                    //                    info <<"\n" <<	"TOTAL MILLIS SKIPPED: " << totalAmountSkipped;
                    //                    ofDrawBitmapStringHighlight(omxPlayer->getInfo() + info.str(), 60, 60, ofColor(ofColor::black, 90), ofColor::yellow);
                }
            }
        }
    }
}

string& getStringFromFile(const string& filename, string& buffer) {
    ifstream in(filename.c_str(), ios_base::binary | ios_base::ate);
    in.exceptions(ios_base::badbit | ios_base::failbit | ios_base::eofbit);
    buffer.resize(in.tellg());
    in.seekg(0, ios_base::beg);
    in.read(&buffer[0], buffer.size());
    return buffer;
}

void WebThread::urlResponse(ofHttpResponse &response){
    if (response.status == 200){
        auto asset = downloadQueue.front();
        cout << "Finish download of : " << asset->name() << endl;
        
        ofFile file(assetPath() + "dl_" + asset->md5name());
        
        if (file.exists()) {
            asset->localPath = assetPath() + asset->md5name();
            if (file.renameTo(asset->localPath, false, true)){
                 asset->isAvailable = true;
            }
            else {
                cout << "error renaming file for : " << asset->name() << endl;
            }
        }
        else {
           cout << "error can't find temp file for : " << asset->name() << endl; 
        }
        
        downloadQueue.erase(downloadQueue.begin());
        downloading = false;
        ofUnregisterURLNotification(this);
    }
    else if (response.status == 302){
        auto asset = downloadQueue.front();
        
        string path = assetPath() + "dl_" + asset->md5name();
        
        string answer;
        getStringFromFile(path, answer);
        
        cout << "answer : " << answer << endl;
        
        auto spl = ofSplitString(answer, "The resource was found at ");
        spl = ofSplitString(spl[1], ";");
        //string redirect = answer.substr(answer.find_last_of("http"));
        //redirect = redirect.substr(0, redirect.find_last_of(";"));
        
        cout << "Redirect download to : " << spl[0] << endl;
        
        loader.saveAsync(spl[0], path);
    }
    else {
        cout << "Error: " << response.status << " " << response.error << endl;
        if (response.status != -1) {
            downloadQueue.erase(downloadQueue.begin());
            downloading = false;
        }
    }
}