#include "ofApp.h"

using namespace ofxCv;

void ofApp::setup() {
    //    Initial setup, I did not change anything here
    ofSetVerticalSync(true);
    ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
    cloneVisible = true;
    
    //    load video
//    vidPlayer.load("botoxHead.mp4");
    
    //    initiate camera
    cam.initGrabber(640, 480);
    cloneReady = false;
    camTracker.setup();                                        // setup the facecamTracker
    camTracker.setRescale(.5);
    camTracker.setIterations(200);
    camTracker.setClamp(10);
    camTracker.setTolerance(.1);
    camTracker.setAttempts(25);
    srcTracker.setup();
    finder.setup("haarcascade_frontalface_default.xml");    //setup basic opencv face detection (the box)
    srcFinder.setup("haarcascade_frontalface_default.xml");
    finder.setPreset(ObjectFinder::Accurate);               //set it to very accurate
    srcFinder.setPreset(ObjectFinder::Accurate);               //set it to very accurate
    clone.setup(cam.getWidth(), cam.getHeight());
    ofFbo::Settings settings;
    settings.width = cam.getWidth();
    settings.height = cam.getHeight();
    maskFbo.allocate(settings);
    srcFbo.allocate(settings);
    srcTracker.setup();
    srcTracker.setIterations(25);
    srcTracker.setAttempts(4);
    
    faces.allowExt("jpg");
    faces.allowExt("png");
    faces.listDir("faces");
    currentFace = 0;
    if(faces.size()!=0){
        loadForehead(faces.getPath(currentFace));
    }
    
    foreHeadSize = 17;
    
//        vidPlayer.play();
}


void ofApp::update() {
    cam.update();                                           //update camera
//        vidPlayer.update();
//        vidPlayer.setSpeed(0.2);
    if(cam.isFrameNew()) {                                  //check for new frame
//            if(vidPlayer.isFrameNew()){
        camTracker.update(toCv(cam));                          //send camera frame in Cv format to tracker
        botoxMe();
        position = camTracker.getPosition();                   //retrieve position
        scale = camTracker.getScale();                         //retrieve scale of face
        orientation = camTracker.getOrientation();             //retrieve orientation of face
        rotationMatrix = camTracker.getRotationMatrix();       //retrieve rotation
//        finder.update(cam);                                 //update standard face detection
        
    }
}

void ofApp::draw() {
//    vidPlayer.draw(0,0);
    if(src.getWidth() > 0 && cloneReady && cloneVisible) {
        clone.draw(0, 0);
//        drawForeheadLine();
//        ofMesh mesh = findForeheadCam();
//        vector<ofVec3f> test = mesh.getVertices();
//        int n = mesh.getNumVertices();
//        for(int i = 0; i < n; i++){
//            ofDrawBitmapString(ofToString(i), test[i]);
//        }
//        ofNoFill();
//        mesh.drawWireframe();
    } else {
        cam.draw(0, 0);
    }
    // draw the camera and framerate
    //	cam.draw(0, 0);
    
    ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, 20);
    
    //draw the simple box
    //	finder.draw();
    
    
    //facetracker part
    if(camTracker.getFound()) { //only works if we have a face
        ofSetLineWidth(1);
           }
}

void ofApp::botoxMe(){
    //    Function for adding mask over forehead to replace wrinkles based on neutral face
    
    cloneReady = camTracker.getFound();
    if(cloneReady) {
        camMesh = findForeheadCam();
        camMesh.clearTexCoords();
        camMesh.addTexCoords(srcPoints);
        
        maskFbo.begin();
        ofClear(0, 255);
        camMesh.draw();
        maskFbo.end();
        
        srcFbo.begin();
        ofClear(0, 255);
        src.bind();
        camMesh.draw();
        src.unbind();
        srcFbo.end();
        
        clone.setStrength(8);
        clone.update(srcFbo.getTexture(), cam.getTexture(), maskFbo.getTexture());
    }
}

void ofApp::loadForehead(string face){
    src.load(face);
    if(src.getWidth() > 0) {
        srcTracker.update(toCv(src));
        ofMesh m = findForeheadSrc();
        srcPoints.resize(m.getNumVertices());
        for(int i = 0 ; i < srcPoints.size(); i++){
            srcPoints[i] = ofVec2f(m.getVertex(i));
        }
    }
}

void ofApp::dragEvent(ofDragInfo dragInfo) {
    loadForehead(dragInfo.files[0]);
}

ofMesh ofApp::findForeheadCam(){
    
    ofVec2f le1 = camTracker.getImagePoint(17);      // retrieve coordinates of left eyebrow out
    ofVec2f le2 = camTracker.getImagePoint(18);      // retrieve coordinates of left eyebrow
    ofVec2f le3 = camTracker.getImagePoint(19);      // retrieve coordinates of left eyebrow
    ofVec2f le4 = camTracker.getImagePoint(20);      // retrieve coordinates of left eyebrow
    ofVec2f le5 = camTracker.getImagePoint(21);      // retrieve coordinates of left eyebrow in
    ofVec2f nb = camTracker.getImagePoint(27);      // retrieve coordinates of nose bridge
    ofVec2f re1 = camTracker.getImagePoint(26);      // retrieve coordinates of right eyebrow out
    ofVec2f re2 = camTracker.getImagePoint(25);      // retrieve coordinates of right eyebrow
    ofVec2f re3 = camTracker.getImagePoint(24);      // retrieve coordinates of right eyebrow
    ofVec2f re4 = camTracker.getImagePoint(23);      // retrieve coordinates of right eyebrow
    ofVec2f re5 = camTracker.getImagePoint(22);      // retrieve coordinates of right eyebrow in
    ofVec2f ls = camTracker.getImagePoint(0);      // left side of the face
    ofVec2f rs = camTracker.getImagePoint(16);     // right side of the face
    float foreheadTop = nb.y-(foreHeadSize*camTracker.getScale());
    int topResolution = 11;
    float bottomResolution = 5;
        ofPolyline poly;
    poly.curveTo(ls.x, ls.y,0, topResolution);   //from left side
    poly.curveTo(ls.x, ls.y-((ls.y-foreheadTop)/2),0, topResolution);   //to left top side 1
    poly.curveTo(ls.x + ((nb.x-ls.x)/2), foreheadTop,0, topResolution);
    poly.curveTo(nb.x,foreheadTop,0, topResolution);    //to top of the head
    poly.curveTo(rs.x - ((rs.x-nb.x)/2), foreheadTop,0, topResolution);
    poly.curveTo(rs.x, rs.y-((ls.y-foreheadTop)/2),0, topResolution);   //to right top side
    poly.curveTo(rs.x, rs.y,0, topResolution);   //to right side
    poly.addVertex(rs.x, rs.y);   //to right side
    poly.addVertex(re1.x, re1.y);     //to right eye
    poly.curveTo(re1.x, re1.y,0, bottomResolution);     //to right eye
    
    poly.curveTo(re2.x, re2.y,0, bottomResolution);     //to right eye
    poly.curveTo(re3.x, re3.y,0, bottomResolution);     //to right eye
    poly.curveTo(re4.x, re4.y,0, bottomResolution);     //to right eye
    poly.curveTo(re5.x, re5.y,0, bottomResolution);     //to right eye
    poly.curveTo(nb.x, nb.y,0, bottomResolution);     //to nosebridge
    poly.curveTo(le5.x, le5.y,0, bottomResolution);     //to left eye
    poly.curveTo(le4.x, le4.y,0, bottomResolution);     //to left eye
    poly.curveTo(le3.x, le3.y,0, bottomResolution);     //to left eye
    poly.curveTo(le2.x, le2.y,0, bottomResolution);     //to left eye
    poly.curveTo(le1.x, le1.y,0, bottomResolution);     //to left eye
    poly.addVertex(le1.x, le1.y);     //to left eye
    
    poly.addVertex(ls.x, ls.y);   //from left side
//        poly.close();
        poly.draw();
        vector<ofVec3f> test = poly.getVertices();
        vector<ofVec2f> test2;
        test2.resize(test.size());
        for(int i = 0 ; i < test.size(); i++){
            test2[i] = ofVec2f(test[i]);
        }
        
        ofMesh mesh = camTracker.getSpecificMesh2(test2);
    return mesh;

}

ofMesh ofApp::findForeheadSrc(){
    
    ofVec2f le1 = srcTracker.getImagePoint(17);      // retrieve coordinates of left eyebrow out
    ofVec2f le2 = srcTracker.getImagePoint(18);      // retrieve coordinates of left eyebrow
    ofVec2f le3 = srcTracker.getImagePoint(19);      // retrieve coordinates of left eyebrow
    ofVec2f le4 = srcTracker.getImagePoint(20);      // retrieve coordinates of left eyebrow
    ofVec2f le5 = srcTracker.getImagePoint(21);      // retrieve coordinates of left eyebrow in
    ofVec2f nb = srcTracker.getImagePoint(27);      // retrieve coordinates of nose bridge
    ofVec2f re1 = srcTracker.getImagePoint(26);      // retrieve coordinates of right eyebrow out
    ofVec2f re2 = srcTracker.getImagePoint(25);      // retrieve coordinates of right eyebrow
    ofVec2f re3 = srcTracker.getImagePoint(24);      // retrieve coordinates of right eyebrow
    ofVec2f re4 = srcTracker.getImagePoint(23);      // retrieve coordinates of right eyebrow
    ofVec2f re5 = srcTracker.getImagePoint(22);      // retrieve coordinates of right eyebrow in
    ofVec2f ls = srcTracker.getImagePoint(0);      // left side of the face
    ofVec2f rs = srcTracker.getImagePoint(16);     // right side of the face
    float foreheadTop = nb.y-(foreHeadSize*camTracker.getScale());
    int topResolution = 11;
    float bottomResolution = 5;
    ofPolyline poly;
    poly.curveTo(ls.x, ls.y,0, topResolution);   //from left side
    poly.curveTo(ls.x, ls.y-((ls.y-foreheadTop)/2),0, topResolution);   //to left top side 1
    poly.curveTo(ls.x + ((nb.x-ls.x)/2), foreheadTop,0, topResolution);
    poly.curveTo(nb.x,foreheadTop,0, topResolution);    //to top of the head
    poly.curveTo(rs.x - ((rs.x-nb.x)/2), foreheadTop,0, topResolution);
    poly.curveTo(rs.x, rs.y-((ls.y-foreheadTop)/2),0, topResolution);   //to right top side
    poly.curveTo(rs.x, rs.y,0, topResolution);   //to right side
    poly.addVertex(rs.x, rs.y);   //to right side
    poly.addVertex(re1.x, re1.y);     //to right eye
    poly.curveTo(re1.x, re1.y,0, bottomResolution);     //to right eye
    
    poly.curveTo(re2.x, re2.y,0, bottomResolution);     //to right eye
    poly.curveTo(re3.x, re3.y,0, bottomResolution);     //to right eye
    poly.curveTo(re4.x, re4.y,0, bottomResolution);     //to right eye
    poly.curveTo(re5.x, re5.y,0, bottomResolution);     //to right eye
    poly.curveTo(nb.x, nb.y,0, bottomResolution);     //to nosebridge
    poly.curveTo(le5.x, le5.y,0, bottomResolution);     //to left eye
    poly.curveTo(le4.x, le4.y,0, bottomResolution);     //to left eye
    poly.curveTo(le3.x, le3.y,0, bottomResolution);     //to left eye
    poly.curveTo(le2.x, le2.y,0, bottomResolution);     //to left eye
    poly.curveTo(le1.x, le1.y,0, bottomResolution);     //to left eye
    poly.addVertex(le1.x, le1.y);     //to left eye
    
    poly.addVertex(ls.x, ls.y);   //from left side
//    poly.close();
    
    vector<ofVec3f> test = poly.getVertices();
    vector<ofVec2f> test2;
    test2.resize(test.size());
    for(int i = 0 ; i < test.size(); i++){
        test2[i] = ofVec2f(test[i]);
    }
    
    ofMesh mesh = srcTracker.getSpecificMesh2(test2);
    
    return mesh;
    
}

void ofApp::drawForeheadLine(){
    ofVec2f le1 = camTracker.getImagePoint(17);      // retrieve coordinates of left eyebrow out
    ofVec2f le2 = camTracker.getImagePoint(18);      // retrieve coordinates of left eyebrow
    ofVec2f le3 = camTracker.getImagePoint(19);      // retrieve coordinates of left eyebrow
    ofVec2f le4 = camTracker.getImagePoint(20);      // retrieve coordinates of left eyebrow
    ofVec2f le5 = camTracker.getImagePoint(21);      // retrieve coordinates of left eyebrow in
    ofVec2f nb = camTracker.getImagePoint(27);      // retrieve coordinates of nose bridge
    ofVec2f re1 = camTracker.getImagePoint(26);      // retrieve coordinates of right eyebrow out
    ofVec2f re2 = camTracker.getImagePoint(25);      // retrieve coordinates of right eyebrow
    ofVec2f re3 = camTracker.getImagePoint(24);      // retrieve coordinates of right eyebrow
    ofVec2f re4 = camTracker.getImagePoint(23);      // retrieve coordinates of right eyebrow
    ofVec2f re5 = camTracker.getImagePoint(22);      // retrieve coordinates of right eyebrow in
    ofVec2f ls = camTracker.getImagePoint(0);      // left side of the face
    ofVec2f rs = camTracker.getImagePoint(16);     // right side of the face
    float foreheadTop = nb.y-(foreHeadSize*camTracker.getScale());
    int topResolution = 11;
    float bottomResolution = 5;
    ofPolyline poly;
    poly.curveTo(ls.x, ls.y,0, topResolution);   //from left side
    poly.curveTo(ls.x, ls.y-((ls.y-foreheadTop)/2),0, topResolution);   //to left top side 1
    poly.curveTo(ls.x + ((nb.x-ls.x)/2), foreheadTop,0, topResolution);
    poly.curveTo(nb.x,foreheadTop,0, topResolution);    //to top of the head
    poly.curveTo(rs.x - ((rs.x-nb.x)/2), foreheadTop,0, topResolution);
    poly.curveTo(rs.x, rs.y-((ls.y-foreheadTop)/2),0, topResolution);   //to right top side
    poly.curveTo(rs.x, rs.y,0, topResolution);   //to right side
    poly.addVertex(rs.x, rs.y);   //to right side
    poly.addVertex(re1.x, re1.y);     //to right eye
    poly.curveTo(re1.x, re1.y,0, bottomResolution);     //to right eye
    
    poly.curveTo(re2.x, re2.y,0, bottomResolution);     //to right eye
    poly.curveTo(re3.x, re3.y,0, bottomResolution);     //to right eye
    poly.curveTo(re4.x, re4.y,0, bottomResolution);     //to right eye
    poly.curveTo(re5.x, re5.y,0, bottomResolution);     //to right eye
    poly.curveTo(nb.x, nb.y,0, bottomResolution);     //to nosebridge
    poly.curveTo(le5.x, le5.y,0, bottomResolution);     //to left eye
    poly.curveTo(le4.x, le4.y,0, bottomResolution);     //to left eye
    poly.curveTo(le3.x, le3.y,0, bottomResolution);     //to left eye
    poly.curveTo(le2.x, le2.y,0, bottomResolution);     //to left eye
    poly.curveTo(le1.x, le1.y,0, bottomResolution);     //to left eye
    poly.addVertex(le1.x, le1.y);     //to left eye
    
    poly.addVertex(ls.x, ls.y);   //from left side
//    poly.close();
    poly.draw();
    vector<ofVec3f> test = poly.getVertices();
    int n = poly.size();
        for(int i = 0; i < n; i++){
            ofDrawBitmapString(ofToString(i), test[i]);
        }
}

void ofApp::keyPressed(int key) {
    if(key == 'r') {
        camTracker.reset();
    }
    if(key == 'v') {
        cloneVisible = !cloneVisible;
    }
    if(key == 'f') {
        // decrease forehead size
        foreHeadSize = foreHeadSize - 0.25;
    }
    if(key == 'F') {
        // increase forehead size
        foreHeadSize = foreHeadSize + 0.25;
    }
}
