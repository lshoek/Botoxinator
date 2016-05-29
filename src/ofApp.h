#pragma once

#include "ofMain.h"
#include "ofxCv.h"
using namespace ofxCv;
using namespace cv;

#include "ofxFaceTracker.h"
#include "Clone.h"
#include "ofxFaceTrackerThreaded.h"

class ofApp : public ofBaseApp {
public:
	void setup();
	void update();
	void draw();
	void keyPressed(int key);
    void botoxMe();
    void loadForehead(string face);
    void dragEvent(ofDragInfo dragInfo);
    
    ofMesh findForeheadCam();
    ofMesh findForeheadSrc();
    void drawForeheadLine();
	ofVideoGrabber cam;
	ofxFaceTrackerThreaded camTracker;
    ofxFaceTracker srcTracker;
    ofVideoPlayer vidPlayer;
    
    ObjectFinder finder;
    ObjectFinder srcFinder;
	ofVec2f position;
	float scale;
	ofVec3f orientation;
	ofMatrix4x4 rotationMatrix;
	
	Mat translation, rotation;
	ofMatrix4x4 pose;
    
    //image overlay stuff
    bool cloneReady;
    Clone clone;
    ofFbo srcFbo, maskFbo;
    ofImage src;
    vector<ofVec2f> srcPoints;
    ofDirectory faces;
    int currentFace, strength;
    bool cloneVisible;
    
    float foreHeadSize;
    ofMesh camMesh;

};
