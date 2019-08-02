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
    
	ofPolyline getForeheadPoly(const ofxFaceTracker& t);
    ofMesh findForehead(const ofxFaceTracker& t);

	void drawForeheadPoints();

    bool showLines;
    bool isRecording;
    bool detectionFailed;

    ofVideoGrabber cam;
	ofxFaceTracker camTracker;
    ofxFaceTracker srcTracker;
    ofVideoPlayer vidPlayer;
    bool pause;
    
    void saveFrame();
    ObjectFinder finder;
    ObjectFinder srcFinder;
	ofVec2f position;
	float scale;
	ofVec3f orientation;
	ofMatrix4x4 rotationMatrix;
    int frameCounter;
	Mat translation, rotation;
	ofMatrix4x4 pose;
    string vidToLoad;
    bool drawLines;

    //image overlay stuff
    bool cloneReady;
    Clone clone;
    ofFbo srcFbo, maskFbo;
    ofImage src, frame;
    vector<glm::vec2> srcPoints;
	vector<glm::vec2> debugPoints;
    
    ofDirectory faces;
    int currentFace, strength;
    bool cloneVisible;
    
    float foreHeadSize;
	ofMesh srcMesh;
    ofMesh camMesh;
    int cloneStrength;

    bool USECAM;
	bool DEBUG_FOREHEAD;
	bool DEBUG_MESHES;
};
