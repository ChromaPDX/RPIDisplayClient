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
#include <sys/syscall.h>

static string appPath = "/home/pi/OmsiDisplay";
static string appVersion = "0.91";

#define LIVE 1

void PlaylistController::runOnMainQueue(std::function<void(void)> operation){
    if (syscall(SYS_gettid) == mainQueueId){
        printf("** dispatch : already on main queue \n");
        operation();
        return;
    }
    printf("main queue dispatch %ld -> %ld \n", syscall(SYS_gettid), mainQueueId);
    mainLock.lock();
    mainQueue.push(operation);
    mainLock.unlock();
}

queue<std::function<void(void)>> PlaylistController::mainQueue;
std::mutex PlaylistController::mainLock;
long PlaylistController::mainQueueId = 0;

string assetPath(){
    return ofToDataPath("./assets/", false);
}

void PlaylistController::setup(){
    deviceId = Poco::Environment::nodeId();
    
    ofEnableAlphaBlending();
    
    printf("setup playlist \n");
    
    webThread.startThread();
    bgThread.startThread();
    parseThread.startThread();
    
    mainQueueId = syscall(SYS_gettid);
    
    printf("main queue id: %ld \n", mainQueueId);
    printf("process th id: %ld \n", getpid());
    
    runOnMainQueue([this](){
        getNextItem();
    });
}

void PlaylistController::fetchData(){
    
    //printf("fetch data in background \n");
    
    webThread.addOperation([this](){
        
        printf("fetching . . . \n");
        
        Json::Value defaultValue;
        
        shared_ptr<Json::Value> response(parseController.fetchAll("Display"));
        
        if (response){
            
            auto results = response->get("results", defaultValue);
            
            vector<shared_ptr<DisplayClient>>displays;
            
            for (auto& value : results){
                
                //printf("fetched result: %s \n", value.toStyledString().c_str());
                
                if (value["deviceId"].asString() == deviceId){
                    me = DisplayClient::fromJson(value);
                }
                
                displays.push_back(DisplayClient::fromJson(value));
            }
            
            if (!me) {
                parseController.createNewDisplay(deviceId);
                return true;
            }
            
            else {
                
                string cachedFunction = me->callCustomFunction;
                me->callCustomFunction = "";
                printf("update me : \n");
                
                uploadScreenShots();
                
                me->loopsSinceBoot = loopsSinceBoot;
                me->failedFetchRequests = failedFetchRequests;
                me->appVersion = appVersion;
                
                auto result = parseController.updateObject(me);
                
                if (result){
                    me->readJson(*result);
                    if (cachedFunction != ""){
                        printf("call custom function : %s \n", cachedFunction.c_str());
                        if (cachedFunction == "reboot"){
                            runOnMainQueue([]{
                                printf("force restart app now !! \n");
                                ofExit(1);
                            });
                        }
                        else if (cachedFunction == "update"){
                            printf("queue app update !! \n");
                            webThread.addOperation([this]{
                                softwareUpdate();
                                return true;
                            });
                        }
                    }
                    auto items = fetchItems();
                    if (items) {
                        runOnMainQueue([this, items]{
                            savePlaylist(items);
                            setItems(items, true);
                        });
                        loopsSinceBoot++;
                        return true;
                    }
                }
            }
            
        }
        
        printf("network fetch failed . . . \n");
        
        // CATCH
        failedFetchRequests++;
        
        auto items = loadCachedPlaylist();
        
        if (items) {
            runOnMainQueue([this, items]{
                setItems(items, false);
                printf("loaded cached playlist ! \n");
            });
        }
        
        return true;
    });
    
    printf("queued fetch \n");
    
}

void PlaylistController::softwareUpdate(){
    
    printf("begin software update . . . \n");
    
    Json::Value defaultValue;
    Json::Value versionToUpdate;
    
    auto response = parseController.fetchAll("SoftwareUpdate");
    
    auto results = response->get("results", defaultValue);
    
    float version = 0.0;
    
    for (auto& updateFile : results){
        float sVersion = updateFile["version"].asFloat();
        if  (sVersion > version){
            version = sVersion;
            versionToUpdate = updateFile;
        }
    }
    
    if (!versionToUpdate["url"].isNull()){
        
        auto asset = make_shared<Asset>();
        
        asset->url = versionToUpdate["url"].asString();
        asset->checksum = versionToUpdate["checksum"].asString();
        asset->version = versionToUpdate["version"].asFloat();
        
        printf("queue download : update -> v %1.2f \n", asset->version);
        
        asset->completionBlock = [asset](){
            
            // main queue
            
            ofDirectory assetDirectory(ofToDataPath("./assets", false));
            
            assetDirectory.listDir();
            
            auto allFiles = assetDirectory.getFiles();
            
            for (auto& file : allFiles){
                
                if (file.getFileName() == asset->md5name()){
                    
                    long long checksum = atoll(asset->checksum.c_str());
                    long long fileSize = file.getSize();
                    
                    printf("file exists : compare filesize %lld : %lld expected \n", fileSize, checksum);
                    
                    if (fileSize == checksum){
                        
                        asset->isAvailable = true;
                        asset->localPath = file.getAbsolutePath();
                        
                        printf("successfully downloaded software v %1.2f \n", asset->version);
                        printf("file path: %s \n", asset->localPath.c_str());
                        
                        ofFile displayApp = ofFile(appPath);
                        if (displayApp.exists()){
                            printf("replacing main software . . . \n");
                            
//                            std::ostringstream vv;
//                            vv << myFloat;
//                            std::string s(ss.str());
                            
                            string archivePath = appPath+"_archive_"+appVersion;
                            displayApp.moveTo(archivePath);
                            
                            if (file.moveTo(appPath)){
                                printf("set permissions \n");
                                string pCommand = "chmod +X " + appPath;
                                
                                std::system(pCommand.c_str());
                                
                                printf("that went splendidly, let's reboot in 5 \n");
                                sleep(5);
                                ofExit(0);
                            }
                            else {
                                printf("uh-oh, can't move new file, restoring original \n");
                                displayApp.moveTo(appPath);
                            }
                        }
                        else {
                            printf("uh-oh, can't find main software, maybe u f'ed up? \n");
                        }
                    }
                    else {
                        printf("invalid or incomplete download for %s , re-downloading \n", asset->name().c_str());
                        if (!file.remove()){
                            printf("error deleting file ! %s \n", asset->name().c_str());
                        }
                        
                    }
                }
            }
            
        };
        
        // we're on webThread so just push
        webThread.queueDownload(asset);
    }
}

bool validateAsset(shared_ptr<Asset> asset){
    
    timePoint now = std::chrono::system_clock::now();
    
    if (!asset->enabled) {
        cout << "asset is disabled" << endl;
        return false;
    }
    
    std::time_t epoch_add = std::chrono::system_clock::to_time_t(asset->addDate);
    std::time_t epoch_remove = std::chrono::system_clock::to_time_t(asset->removeDate);
    
    if (epoch_add != 0 && asset->addDate > now){
        cout << "failed on addDate : " << time_point_to_string(asset->addDate) << " < " << time_point_to_string(now) << endl;
        return false;
    }
    
    if (epoch_remove != 0 && asset->removeDate < now){
        cout << "failed on removeDate : " << time_point_to_string(now) << " < " << time_point_to_string(asset->removeDate) << endl;
        return false;
    }
    
    return true;
}

shared_ptr<Json::Value> PlaylistController::fetchItems() {
    auto result = parseController.callFunction("itemsForDisplay", me->toJson());
    
    if (result && result->size()) {
        //printf("my items : %s \n", result->toStyledString().c_str());
        return result;
    }
    else {
        printf("error fetching my items : \n");
        return nullptr;
    }
}

void PlaylistController::setItems(shared_ptr<Json::Value> playlistItems, bool isOnline) {
    
    if (!playlistItems){
        // setting to nil
        // default asset will be loaded next playlist loop
        return;
    }
    
    playlist = make_shared<Playlist>();
    
    Json::Value defaultValue;
    
    auto results = playlistItems->get("result", defaultValue);
    
    if (results.size() && playlist->items.size()) {
        printf("clear items \n");
        playlist->items.clear();
    }
    
    ofDirectory assetDirectory(ofToDataPath("./assets", false));
    
    assetDirectory.listDir();
    
    auto allFiles = assetDirectory.getFiles();
    
    for (auto& value : results){
        
        auto item = PlaylistItem::fromJson(value);
        
        for (auto& file : allFiles){
            
            if (file.getFileName() == item->asset->md5name()){
                
                long long checksum = atoll(item->asset->checksum.c_str());
                long long fileSize = file.getSize();
                
                printf("file exists : compare filesize %lld : %lld expected \n", fileSize, checksum);
                
                if (fileSize == checksum){
                    item->asset->isAvailable = true;
                    item->asset->localPath = file.getAbsolutePath();
                }
                else {
                    printf("invalid or incomplete download for %s , re-downloading \n", item->asset->name().c_str());
                    if (!file.remove()){
                        printf("error deleting file ! %s \n", item->asset->name().c_str());
                    }
                    
                }
            }
        }
        
        if (!item->asset->isAvailable) {
            if (isOnline){
                webThread.queueDownload(item->asset);
            }
        }
        
        if (validateAsset(item->asset) && item->asset->isAvailable) {
            playlist->items.push_back(item);
            cout << "validated asset : " << item->asset->name() << endl;
        }
        
        else {
            cout << "asset invalid : " << item->asset->name() << endl;
        }
        
    }
    
    sort(playlist->items.begin(), playlist->items.end(), [](shared_ptr<PlaylistItem> a, shared_ptr<PlaylistItem> b) -> bool
         {
             return a->index < b->index;
         });
    
    for (auto& player : players){
        player.listener = this;
    }
    
    printf("finished set items \n");
}

bool PlaylistController::savePlaylist(shared_ptr<Json::Value> playlistItems){
    string path = assetPath() + "cachedPlaylist.txt";
    
    ofFile file (path);
    
    if (file.exists()){
        file.remove();
    }
    
    if (playlistItems) {
        std::ofstream out(path);
        out << playlistItems->toRawString();
        out.close();
        return true;
    }
    return false;
}

shared_ptr<Json::Value> PlaylistController::loadCachedPlaylist() {
    
    string path = assetPath() + "cachedPlaylist.txt";
    
    ofFile file (path);
    
    if (file.exists()){
        std::ifstream t(path);
        std::string str((std::istreambuf_iterator<char>(t)),
                        std::istreambuf_iterator<char>());
        
        
        
        auto playlist = make_shared<Json::Value>();
        
        Json::Reader reader;
        
        if (!reader.parse( str, *playlist.get() ))
        {
            cout << "Unable to parse string: " << reader.getFormattedErrorMessages();
            return shared_ptr<Json::Value>();
        }
        
        cout << "read cached playist" << str << endl;
        
        return playlist;
    }
    
    cout << "cached playist doesn't exist: " << path << endl;
    
    return shared_ptr<Json::Value>();
}

shared_ptr<Playlist> PlaylistController::defaultPlaylist() {
    
    auto item = make_shared<PlaylistItem>(AssetTypePhoto, assetPath() + "default.jpg", 30);
    
    auto playlist = make_shared<Playlist>();
    
    playlist->items.push_back(item);
    
    return playlist;
}

void PlaylistController::getNextItem(){
    if (counter == 0) {
        if (!webThread.downloading()) {
            currentItem = nullptr;
            fetchData();
        }
        else {
            printf("download in progress, skipping fetch \n");
        }
    }
    
    // force playlist to default, if none present
    if (!playlist || !playlist->items.size()) {
        printf("load default image ! \n");
        playlist = defaultPlaylist();
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
        return;
    }
    
    if (currentItem && !currentItem->asset->isAvailable) {
        printf("asset : %s is unavailable / downloading", currentItem->asset->name().c_str());
        
        currentItem.reset();
        int availableCount = 0;
        for (auto item : playlist->items) {
            if (item->asset->isAvailable) availableCount++;
        }
        if (availableCount == 0) {
            printf("(%d of %d) wait to start til at least one asset is downloaded \n", availableCount, playlist->items.size());
            bgThread.addOperation([this]{
                sleep(10);
                int availableCount = 0;
                for (auto item : playlist->items) {
                    if (item->asset->isAvailable) availableCount++;
                }
                if (availableCount != 0) {
                    printf("ok, downloaded, let's go ! \n");
                    runOnMainQueue([this](){getNextItem();});
                    return true;
                }
                else {
                    printf("(%d of %d) wait to start til at least two assets are downloaded \n", availableCount, playlist->items.size());
                }
                return false;
            });
            return;
        }
        getNextItem();
    }
    
}

void PlaylistController::update() { // MAIN THREAD
    
    mainLock.lock();
    
    if (mainQueue.size()){
        mainQueue.front()();
        mainQueue.pop();
    }
    
    mainLock.unlock();
    
    // ONLY USE 'thread local' vars here down -->>
    
    bool hasAsset = false;
    
    if (currentItem){
        
        switch (transitionState){
                
            case TransitionLoad:
                
                if (playlist->items.size() == 1 && players[currentPlayer].playlistItem()) {
                    if (players[currentPlayer].playlistItem()->asset->url == playlist->items[0]->asset->url) {
                        // Edge Case, only one item on playlist and already loaded
                        players[currentPlayer].playlistItem()->duration = 60;
                        transitionState = TransitionPlay;
                        printf("single slide - sleep 60 - refresh \n");
                        return;
                    }
                    printf("single item, but not equal ??\n");
                }
                
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
                transitionState = TransitionPlay;
                break;
                
            case TransitionPlay:
                
                if (currentItem->duration != 0 || currentItem->asset->type == AssetTypePhoto){
                    if (currentItem->duration < 3) currentItem->duration = 3; // minimum 3 seconds for slides
                    bgThread.addOperation([this](){
                        sleep(currentItem->duration / 2);
                        runOnMainQueue([this](){takeScreenShot(currentItem->index);});
                        sleep(currentItem->duration / 2);
                        runOnMainQueue([this](){getNextItem();});
                        return true;
                    });
                }
                else {
                    bgThread.addOperation([this]{
                        sleep(5);
                        runOnMainQueue([this](){takeScreenShot(currentItem->index);});
                        // add timeout for video
                        if (currentItem->asset->type == AssetTypeVideo && 0){ // forced off
                            shared_ptr<Asset> videoAsset = currentItem->asset;
                            float duration = PlaylistItemPlayer::omxPlayer->getDurationInSeconds();
                            //if (duration < 5.0f) duration = 5.0f;
                            printf("set watchdog timer for : %d \n %d frames / %f fps \n", (int)duration, PlaylistItemPlayer::omxPlayer->getTotalNumFrames(), PlaylistItemPlayer::omxPlayer->getFPS());
                            sleep((int)duration);
                            // still on video ? getNextItem
                            if (currentItem->asset == videoAsset) {
                                cout << "ERROR invoking watchdog override for video " << videoAsset->name() << endl;
                                runOnMainQueue([this](){getNextItem();});
                            }
                        }
                        return true;
                    });
                }
                
                transitionState = TransitionWaiting;
                break;
                
            case TransitionWaiting:
                //nop
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
    runOnMainQueue([this](){getNextItem();});
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
    
    cout << "grab screen shot" << endl;
    
    shared_ptr<ofImage> screenShot = make_shared<ofImage>();
    screenShot->setUseTexture(false);
    
    screenShot->grabScreen(ofGetWidth() / 4, ofGetHeight() / 4, ofGetWidth() / 2, ofGetHeight() / 2);
    
    bgThread.addOperation([screenShot, frame](){
        screenShot->resize(ofGetWidth() / 4, ofGetHeight() / 4);
        
        string pad = "";
        if (frame < 100) {
            pad = "0";
            if (frame < 10) pad = "00";
        }
        
        char fileName[32];
        sprintf(fileName, "screenShot%s%d.jpg", pad.c_str(), frame);
        
        string path = assetPath() + string(fileName);
        
        cout << "save screen shot : " << fileName << endl;
        
        ofFile file(path);
        
        if (file.exists()) {
            file.remove();
        }
        
        screenShot->saveImage(path, OF_IMAGE_QUALITY_MEDIUM);
        cout << "finish writing screenshot" << endl;
        
        screenShot->clear();
        return true;
    });
}

//--------------------------------------------------------------
#pragma mark - PlaylistItemPlayer
//--------------------------------------------------------------

ofxOMXPlayer* PlaylistItemPlayer::omxPlayer = nullptr;

void PlaylistItemPlayer::loadPlaylistItem(shared_ptr<PlaylistItem> item){
    
    if (_playlistItem) {
        clear();
    }
    
    _playlistItem = item;
    
    if (_playlistItem) {
        
        if (_playlistItem->asset->type == AssetTypePhoto){
            
            if (!image) {
                image = make_shared<ofImage>(_playlistItem->asset->localPath);
            }
            else {
                image->loadImage(_playlistItem->asset->localPath);
            }
            
            //if (image->loadImage()){
            printf("load photo %s \n", _playlistItem->asset->localPath.c_str());
            printf("glTex count: %d \n", image->getTextureReference().getTextureData().textureID);
            //}
            //            else {
            //                printf("error loading: photo %s \n", _playlistItem->asset->localPath.c_str());
            //            }
            
        }
        else if (_playlistItem->asset->type == AssetTypeVideo){
            
            if (loadMovie(_playlistItem)){
                printf("(*) load video %s \n", _playlistItem->asset->localPath.c_str());
            }
            else {
                printf("error loading: video %s \n", _playlistItem->asset->localPath.c_str());
            }
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
                    //omxPlayer->setPaused(true);
                    //omxPlayer->getTextureReference().clear();
                    printf("( ) closing video %s \n", _playlistItem->asset->localPath.c_str());
                    omxPlayer->close();
                    //delete omxPlayer;
                    //omxPlayer = nullptr;
                    // clear existing video, here before fading into new video
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
        alpha = 255;
        // finish transition IN
    }
    else {
        // finish transition OUT
        clear();
    }
}

void PlaylistItemPlayer::clear() {
    if (_playlistItem) {
        if (_playlistItem->asset->type == AssetTypeVideo) {
            // don't clear shared player here, because new video could be already playing
        }
        else if (_playlistItem->asset->type == AssetTypePhoto){
            // RIGHT NOT DELETING IMAGES - BECAUSE RE-ALLOCATION HAS SOME LEAKS?
            
            GLuint texId[1];
            texId[0] = image->getTextureReference().texData.textureID;
            // THIS POS HACK BECAUSE OF ISN'T RELEASING THE GL TEXTURES
            if (texId[0] != 0){
                //printf("clear GL image %d -> ", texId[0]);
                glDeleteTextures(1, texId);
                //printf("%d \n", texId[0]);
                GLenum error = glGetError();
                if (error != 0) {
                    printf("gl error : %d \n", error);
                }
                
            }
            image->clear();
            image.reset();
        }
        
        _playlistItem = nullptr;
        alpha = 0;
    }
}


//--------------------------------------------------------------

bool PlaylistItemPlayer::loadMovie(shared_ptr<PlaylistItem> item)
{
    if (!omxPlayer) {
        printf("** init omx player \n");
        omxPlayer = new ofxOMXPlayer();
        ofSetLogLevel("ofThread", OF_LOG_ERROR);
    }
    
    settings.videoPath = item->asset->localPath;
    settings.enableAudio	  = false;
    settings.useHDMIForAudio = false;	//default true
    settings.enableLooping = (item->duration != 0);		//default true
    settings.enableTexture = true;		//default true
    settings.listener = listener;
    
    if (omxPlayer->setup(settings)){
        printf("setup movie %s \n", item->asset->localPath.c_str());
        return true;
    }
    else {
        printf("error setting up omx player");
        return false;
    }
    
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
            if (image->isAllocated()){
                ofSetColor(255, 255, 255, alpha);
                image->draw(0,0,ofGetWidth(), ofGetHeight());
            }
        }
        else if (_playlistItem->asset->type == AssetTypeVideo){
            if (omxPlayer){
                if (omxPlayer->isTextureEnabled()){
                    ofSetColor(255, 255, 255, alpha);
                    omxPlayer->draw(0, 0, ofGetWidth(), ofGetHeight());
                    
                    //                                        stringstream info;
                    //                                        info <<"\n" << _playlistItem->file;
                    //                                        info <<"\n" <<	"MILLIS SKIPPED: "		<< amountSkipped;
                    //                                        info <<"\n" <<	"TOTAL MILLIS SKIPPED: " << totalAmountSkipped;
                    //                                        ofDrawBitmapStringHighlight(omxPlayer->getInfo() + info.str(), 60, 60, ofColor(ofColor::black, 90), ofColor::yellow);
                }
                else {
                    printf("problem with movie !!");
                    clear();
                    //getNextItem();
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

        //downloadQueue.erase(downloadQueue.begin());
        ofUnregisterURLNotification(this);
        
        if (asset->completionBlock) {
            PlaylistController::runOnMainQueue(asset->completionBlock);
        }
        
        lock();
        downloadQueue.pop();
        _downloading = false;
        unlock();
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
            ofUnregisterURLNotification(this);
            lock();
            downloadQueue.pop();
            _downloading = false;
            unlock();
        }
    }
}