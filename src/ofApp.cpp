#include "ofApp.h"

using namespace ofxCv;

void ofApp::setup() {
	ofSetVerticalSync(true);
	ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
	
	cam.initGrabber(640, 480);
	
	tracker.setup();
    finder.setup("haarcascade_frontalface_default.xml");
    finder.setPreset(ObjectFinder::Accurate);
}


void ofApp::update() {
	cam.update();
	if(cam.isFrameNew()) {
		tracker.update(toCv(cam));
		position = tracker.getPosition();
		scale = tracker.getScale();
		orientation = tracker.getOrientation();
		rotationMatrix = tracker.getRotationMatrix();
        finder.update(cam);
	}
}

void ofApp::draw() {
	ofSetColor(255);
	cam.draw(0, 0);
	ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, 20);
	finder.draw();
	if(tracker.getFound()) {
		ofSetLineWidth(1);
        ofVec2f a = tracker.getImagePoint(18);
        ofVec2f b = tracker.getImagePoint(27);
        ofVec2f c = tracker.getImagePoint(25);
        float leftHeight = tracker.getGesture(ofxFaceTracker::LEFT_EYEBROW_HEIGHT);
        float rightHeight = tracker.getGesture(ofxFaceTracker::RIGHT_EYEBROW_HEIGHT);
        // get coordinates of left and right eyebrow.
        ofBeginShape();
        ofVertex(a.x, a.y);
        ofVertex(b.x, b.y);
        ofVertex(c.x, c.y);
        ofVertex(c.x, c.y-100+(rightHeight*7));
        ofVertex(a.x, a.y-100+(leftHeight*7));
        ofEndShape();
        
		ofSetupScreenOrtho(640, 480, -1000, 1000);
		ofTranslate(640 / 2, 480 / 2);
        
        ofPushMatrix();
        ofScale(5,5,5);
        tracker.getObjectMesh().drawWireframe();
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
