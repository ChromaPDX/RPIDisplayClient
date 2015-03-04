#pragma once

#include "ofMain.h"

#include "PlaylistController.h"



class ofApp : public ofBaseApp{
public:
    
    void setup();
    
    void update();
    void draw();
    
    void keyPressed(int key);
    void keyReleased(int key);
    void mouseMoved(int x, int y);
    void mouseDragged(int x, int y, int button);
    void mousePressed(int x, int y, int button);
    void mouseReleased(int x, int y, int button);
    void windowResized(int w, int h);
    void dragEvent(ofDragInfo dragInfo);
    void gotMessage(ofMessage msg);
    
    // SLOW UPDATE THREADS
    
    // IMAGE DRAWING
    ofxJSONElement flickr;
    void requestFlickr();
    
    unsigned int currentImage = 0;
    float offset = 0;
    float increment = 0;
    bool mode = false;
    bool wait = false;

    PlaylistController playlistController;
    // PLAYBACK
    

};
