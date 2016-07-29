#include "ofApp.h"

using namespace ofxCv;

void ofApp::setup() {
    
    // Use camera, if false it uses the movie. call movie file movie.mp4
    USECAM = false;
    pause = false;
    isRecording = false;
    detectionFailed = false;
    frameCounter = 0;
    drawLines = true;
    //    Initial setup, I did not change anything here
    ofSetVerticalSync(true);
    ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);
    cloneVisible = true;
    cloneStrength = 5;
    //    load video
    showLines = false;
    
    if(USECAM){
        //    initiate camera
        cam.initGrabber(640, 480);
    } else {
        vidToLoad = "a1";
        vidPlayer.load(vidToLoad+".mp4");
        vidPlayer.setLoopState(OF_LOOP_NORMAL);
//        vidPlayer.play();
//        vidPlayer.setPaused(true);
    }
    
    cloneReady = false;
    camTracker.setup();                                        // setup the facecamTracker
    // different settings per case.
    if(USECAM){
        camTracker.setRescale(0.25);
        camTracker.setIterations(100);
        camTracker.setClamp(6);
        camTracker.setTolerance(.3);
        camTracker.setAttempts(25);
    } else {
        camTracker.setRescale(1);
        camTracker.setIterations(400);
        camTracker.setClamp(20);
        camTracker.setTolerance(.01);
        camTracker.setAttempts(2);
    }
    srcTracker.setup();
    srcTracker.setRescale(1);
    srcTracker.setIterations(200);
    srcTracker.setClamp(20);
    srcTracker.setTolerance(.01);
    srcTracker.setAttempts(125);
    // settings for Clone
    ofFbo::Settings settings;
    if(USECAM){
        clone.setup(cam.getWidth(), cam.getHeight());
        settings.width = cam.getWidth();
        settings.height = cam.getHeight();
    }
    else {
        clone.setup(vidPlayer.getWidth(), vidPlayer.getHeight());
        settings.width = vidPlayer.getWidth();
        settings.height = vidPlayer.getHeight();
    }
    
    frame.allocate(vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR);
    
    maskFbo.allocate(settings);
    srcFbo.allocate(settings);
    srcTracker.setup();
    srcTracker.setIterations(25);
    srcTracker.setAttempts(4);
    
    faces.allowExt("jpg");
    faces.allowExt("png");
    faces.listDir("faces");
    // load standard face from folder /faces.
    currentFace = 0;
    if(faces.size()!=0){
        loadForehead(faces.getPath(currentFace));
    }
    
    foreHeadSize = 20; // this is the height of the forehead, needs to be specified per person
    
    sleep(3);
}


void ofApp::update() {
    if(USECAM){
        cam.update();                                           //update camera
    } else {
        vidPlayer.update();                                     // update vidplayer
    }
    
    if(USECAM && cam.isFrameNew()) {                                  //check for new frame
        camTracker.update(toCv(cam));                          //send camera frame in Cv format
        botoxMe();
        position = camTracker.getPosition();                   //retrieve position
        scale = camTracker.getScale();                         //retrieve scale of face
        orientation = camTracker.getOrientation();             //retrieve orientation of face
        rotationMatrix = camTracker.getRotationMatrix();       //retrieve rotation
        
    }
    // different loop for video
    else if(vidPlayer.isFrameNew()){
        detectionFailed = !camTracker.update(toCv(vidPlayer));
        if(!detectionFailed){                          //send camera frame in Cv
            botoxMe();
            // This causes a bug, it doesn't really work. Press 'n' to call nextFrame() manually
            //            vidPlayer.nextFrame();
        } else {
            ofLogNotice("nothing found");
//            detectionFailed = true;
        }
        if(isRecording){
            saveFrame();
            vidPlayer.nextFrame();
//            camTracker.reset();
        }
        
    }
}

void ofApp::draw() {
    if(src.getWidth() > 0 && cloneReady && cloneVisible) {
        clone.draw(0, 0);
        src.draw(640, 0);
        if(showLines){
                drawForeheadLine();                           // press 'l'
        }
        drawForeheadLineSrc();
    } else {
        if(USECAM){
            cam.draw(0, 0);
        } else {
            vidPlayer.draw(0, 0);
        }
    }
    
    if(drawLines){
    ofDrawBitmapString(ofToString((int) ofGetFrameRate()), 10, 20);
    ofDrawBitmapString("Clone Strength: " + ofToString(cloneStrength),10, 40);
    ofDrawBitmapString("Forehead size: " + ofToString(foreHeadSize),10, 60);
    ofDrawBitmapString("frame: " + ofToString(vidPlayer.getCurrentFrame()) + "/"+ofToString(vidPlayer.getTotalNumFrames()),20,380);
    ofDrawBitmapString("duration: " + ofToString(vidPlayer.getPosition()*vidPlayer.getDuration(),2) + "/"+ofToString(vidPlayer.getDuration(),2),20,400);
    ofDrawBitmapString("speed: " + ofToString(vidPlayer.getSpeed(),2),20,420);
    ofDrawBitmapString("is recording: "+ ofToString(isRecording), 20, 440);
    }
}

void ofApp::botoxMe(){
    //    Function for adding mask over forehead to replace wrinkles based on neutral face
    
    cloneReady = camTracker.getFound();
    if(cloneReady) {
        camMesh = findForeheadCam();                    // find the forehead on the cam/vid
        camMesh.clearTexCoords();                       // remove textures
        camMesh.addTexCoords(srcPoints);                // add textures from source
        
        // mask
        maskFbo.begin();
        ofClear(0, 255);
        camMesh.draw();
        maskFbo.end();
        
        // src
        srcFbo.begin();
        ofClear(0, 255);
        src.bind();
        camMesh.draw();
        src.unbind();
        srcFbo.end();
        
        // do some cloning
        clone.setStrength(cloneStrength);
        if(USECAM){
            clone.update(srcFbo.getTexture(), cam.getTexture(), maskFbo.getTexture());
        } else {
            clone.update(srcFbo.getTexture(), vidPlayer.getTexture(), maskFbo.getTexture());
        }
    } else {
        // clone failed.. reset?
        camTracker.reset();
        
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




// This function finds the forehead based on the eyebrows and forehead size.
ofMesh ofApp::findForeheadCam(){
    
    // retrieve all coordinates
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
    
    // determine forehead top location.
    float foreheadTop = nb.y-(foreHeadSize*camTracker.getScale());
    // resolution for curves from point to point
    int topResolution = 11;
    float bottomResolution = 5;
    
    // create the polyline.
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
    
    // get the vertices from the polyline
    vector<ofVec3f> test = poly.getVertices();
    vector<ofVec2f> test2; // we need a 2dimensional vector, not 3
    test2.resize(test.size());
    for(int i = 0 ; i < test.size(); i++){
        test2[i] = ofVec2f(test[i]);
    }
    // now retrieve the mesh from the facetracker for these points, and return it.
    ofMesh mesh = camTracker.getSpecificMesh2(test2);
    return mesh;
    
}


// same as function above, but for source face
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
    
    vector<ofVec3f> test = poly.getVertices();
    vector<ofVec2f> test2;
    test2.resize(test.size());
    for(int i = 0 ; i < test.size(); i++){
        test2[i] = ofVec2f(test[i]);
    }
    
    ofMesh mesh = srcTracker.getSpecificMesh2(test2);
    
    return mesh;
    
}



// same same as above, but this draws the lines of the forehead found.
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
    
    vector<ofVec3f> test = poly.getVertices();
    int n = poly.size();
    for(int i = 0; i < n; i++){
//        ofDrawBitmapString(ofToString(i), test[i]);
        ofDrawCircle(test[i].x, test[i].y, 2);
    }
}

// same same as above, but this draws the lines of the forehead found.
void ofApp::drawForeheadLineSrc(){
    
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
    float foreheadTop = nb.y-(foreHeadSize*srcTracker.getScale());
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
    
    vector<ofVec3f> test = poly.getVertices();
    int n = poly.size();
    ofPushMatrix();
    ofTranslate(640, 0);
    for(int i = 0; i < n; i++){
//        ofDrawBitmapString(ofToString(i), test[i]);
        ofDrawCircle(test[i].x, test[i].y, 2);
    }
    ofPopMatrix();
}

void ofApp::saveFrame(){
    
//    unsigned char* pixels;
    if(!detectionFailed){
    ofPixels pixels;
    pixels.allocate(vidPlayer.getWidth(),vidPlayer.getHeight(),OF_IMAGE_COLOR_ALPHA);
    clone.buffer.readToPixels(pixels);
    frame.setFromPixels(pixels, clone.buffer.getWidth(), clone.buffer.getHeight(), OF_IMAGE_COLOR_ALPHA);
    }
    else{
        frame.setFromPixels(vidPlayer.getPixels(),vidPlayer.getWidth(),vidPlayer.getHeight(), OF_IMAGE_COLOR);
    }
    string imageFileName = ofToString(frameCounter);
    frame.save("data/caps/"+vidToLoad+"/"+imageFileName+".png");
    frameCounter++;
}


void ofApp::keyPressed(int key) {
    
    switch(key){
        case 'r':
            camTracker.reset();
            break;
        case 'R':
            isRecording = !isRecording;
            break;
        case 'v':
            cloneVisible = !cloneVisible;
            break;
        case 'f':
            // decrease forehead size
            foreHeadSize = foreHeadSize - 0.25;
            break;
        case 'F':
            // increase forehead size
            foreHeadSize = foreHeadSize + 0.25;
            break;
        case 'n':
            camTracker.reset();
            break;
        case 'N':
            srcTracker.reset();
            break;
        case 's':
            //decrease  clonestrength
            cloneStrength--;
            cloneStrength = ofClamp(cloneStrength,0,16);
            break;
        case 'S':
            //increase  clonestrength
            cloneStrength++;
            cloneStrength = ofClamp(cloneStrength,0,16);
            break;
            
        case 'p':
            // pause
            pause = !pause;
            vidPlayer.setPaused(pause);
            break;
        case OF_KEY_LEFT:
            vidPlayer.previousFrame();
            ofLogNotice("previousFrame");
            break;
        case OF_KEY_RIGHT:
            camTracker.reset();
            vidPlayer.nextFrame();
            ofLogNotice("nextFrame");
            break;
        case '0':
            vidPlayer.firstFrame();
            break;
        case 'l':
            showLines = !showLines;
            break;
            case 'd':
            drawLines = !drawLines;
    }
}
