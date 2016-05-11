#include "ofApp.h"

using namespace ofxCv;

void ofApp::setup() {
//    Initial setup, I did not change anything here
	ofSetVerticalSync(true);
	ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
	
//    initiate camera
	cam.initGrabber(640, 480);
	
	tracker.setup();                                        // setup the facetracker
    finder.setup("haarcascade_frontalface_default.xml");    //setup basic opencv face detection (the box)
    finder.setPreset(ObjectFinder::Accurate);               //set it to very accurate
}


void ofApp::update() {
	cam.update();                                           //update camera
	if(cam.isFrameNew()) {                                  //check for new frame
		tracker.update(toCv(cam));                          //send camera frame in Cv format to tracker
		position = tracker.getPosition();                   //retrieve position
		scale = tracker.getScale();                         //retrieve scale of face
		orientation = tracker.getOrientation();             //retrieve orientation of face
		rotationMatrix = tracker.getRotationMatrix();       //retrieve rotation
        finder.update(cam);                                 //update standard face detection
	}
}

void ofApp::draw() {
	ofSetColor(255);
    
    // draw the camera and framerate
	cam.draw(0, 0);
	ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, 20);
    
    //draw the simple box
	finder.draw();
    
    //facetracker part
	if(tracker.getFound()) { //only works if we have a face
		ofSetLineWidth(1);
        ofVec2f a = tracker.getImagePoint(18);      // retrieve coordinates of left eyebrow
        ofVec2f b = tracker.getImagePoint(27);      // retrieve coordinates of nose bridge
        ofVec2f c = tracker.getImagePoint(25);      // retrieve coordinates of right eyebrow
        
        // get height of eyebrows
        float leftHeight = tracker.getGesture(ofxFaceTracker::LEFT_EYEBROW_HEIGHT);
        float rightHeight = tracker.getGesture(ofxFaceTracker::RIGHT_EYEBROW_HEIGHT);
        
        // draw detection box
        ofBeginShape();
        ofVertex(a.x, a.y);
        ofVertex(b.x, b.y);
        ofVertex(c.x, c.y);
        ofVertex(c.x, c.y-100+(rightHeight*7)); //hacky implementation of forehead. change height based on
        ofVertex(a.x, a.y-100+(leftHeight*7));  //the height of the eyebrows, higher eyebrows -> lower forehead
        ofEndShape();
        
        // already there from the example to display the wireframe
		ofSetupScreenOrtho(640, 480, -1000, 1000);
		ofTranslate(640 / 2, 480 / 2);
        
        ofPushMatrix();
        ofScale(5,5,5);
//        tracker.getObjectMesh().drawWireframe();
        ofPopMatrix();
        
		applyMatrix(rotationMatrix);
		ofScale(5,5,5);
//		tracker.getObjectMesh().drawWireframe();
	}
}

void ofApp::keyPressed(int key) {
	if(key == 'r') {
		tracker.reset();
	}
}
