#include "ofApp.h"


static array<string, 5> searchTerms = {"omsi", "dinosaurs", "chameleons", "einstein", "raspberry+pi"};
//static array<string, 5> searchTerms = {"ema", "erika+m+anderson", "south+dakota", "william+gibson", "neuromancer"};
static unsigned int searchTerm = 0;

//--------------------------------------------------------------
void ofApp::setup(){
    printf("******** enter setup ****** \n");
    //
    
    ofSetFrameRate(60);

    playlistController.setup();
}


char getRandomChar(){
    static char c = 'A' + rand()%24;
    return c;
}

void gen_random(char *s, const int len) {
    static const char alphanum[] = "abcdefghijklmnopqrstuvwxyz";
    //    "0123456789"
    //    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    
    for (int i = 0; i < len; ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    s[len] = 0;
}

//--------------------------------------------------------------
void ofApp::update(){
    playlistController.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofBackground(0);
    playlistController.draw();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
    
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    
}


//void ofApp::requestFlickr() {
//
//    int length = (rand() % 4) + 4;
//    char search[length];
//    gen_random(search, length-1);
//
//    string s(search);
//
//    std::string flurl = "https://www.flickr.com/services/rest/?method=flickr.photos.search&tags=" + searchTerms[searchTerm] + "&api_key=76fee119f6a01912ef7d32cbedc761bb&format=json&nojsoncallback=1";
//
//    searchTerm++;
//
//    if (!flickr.open(flurl))
//    {
//        ofLogNotice("ofApp::setup") << "Failed to parse JSON";
//    }
//    else {
//        ofLogNotice("ofApp::setup") << flurl;
//    }
//
//    int numImages = MIN(8, flickr["photos"]["photo"].size());
//
//    networkController.lock();
//
//    while (networkController.images.size() < numImages) {
//        networkController.images.push_back(urlImage());
//    }
//
//    for(unsigned int i = 0; i < numImages; i++)
//    {
//        int farm = flickr["photos"]["photo"][i]["farm"].asInt();
//        std::string id = flickr["photos"]["photo"][i]["id"].asString();
//        std::string secret = flickr["photos"]["photo"][i]["secret"].asString();
//        std::string server = flickr["photos"]["photo"][i]["server"].asString();
//        std::string url = "http://farm" + ofToString(farm) + ".static.flickr.com/" + server + "/" + id + "_" + secret + ".jpg";
//
//        networkController.operationQueue.push([=](NetworkController* thread){
//            thread->images[i].dataSource.setUrl(url);
//            return true;
//        });
//    }
//
//    networkController.unlock();
//
//}