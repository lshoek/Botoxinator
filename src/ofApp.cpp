#include "ofApp.h"

using namespace ofxCv;

void ofApp::setup()
{
	// Load settings.ini if available
	appSettings = ofxIniSettings("settings.ini");

	// Use camera, if false it uses the movie. call movie file movie.mp4
	USECAM = appSettings.get("general.use_cam", false);
	AUTO_EXIT = appSettings.get("general.auto_mode", false);
	DEBUG_TEXT = appSettings.get("debugging.show_text", true);
	DEBUG_FOREHEAD = appSettings.get("debugging.show_forehead", false);
	DEBUG_SOURCE = appSettings.get("debugging.show_source", false);
	DEBUG_LINES = appSettings.get("debugging.show_lines", false);
	DEBUG_MESH = appSettings.get("debugging.show_mesh", false);
	isRecording = appSettings.get("general.record_video", false);
	pauseVideo = appSettings.get("general.pause_video", false);
	windowRes = glm::vec2(appSettings.get("general.window_width", 1280), appSettings.get("general.window_height", 800));
	bool windowAutoSize = appSettings.get("general.window_auto", true);

	// instance variables
	detectionFailed = false;
	frameCounter = 0;

	// Initial setup, I did not change anything here
	ofSetVerticalSync(true);
	ofSetDrawBitmapMode(OF_BITMAPMODE_MODEL_BILLBOARD);

	cloneVisible = true;
	cloneStrength = 5;

	if (USECAM) 
	{
		cam.initGrabber(640, 480);
		if (windowAutoSize) ofSetWindowShape(cam.getWidth(), cam.getHeight());
	}
	else 
	{
		vidToLoad = ofFile(args.size() > 1 ? args[1] : "videos/test.mp4");
		
		vidPlayer.load(vidToLoad.getAbsolutePath());
		vidLoaded = vidPlayer.isLoaded();
		if (!vidLoaded) {
			ofSystemAlertDialog("Could not load video: Incorrect filepath or incompatible video file. (" + vidToLoad.getAbsolutePath() + ')');
			ofExit();
		}

		vidPlayer.setLoopState(OF_LOOP_NONE);
		vidPlayer.play();
		vidPlayer.setPaused(pauseVideo);
		if (windowAutoSize) ofSetWindowShape(vidPlayer.getWidth(), vidPlayer.getHeight());
	}
	if (!windowAutoSize) ofSetWindowShape(windowRes.x, windowRes.y);

	cloneReady = false;
	camTracker.setup(); // setup the facecamTracker

	// different settings per case.
	if (USECAM) 
	{
		camTracker.setRescale(appSettings.get("tracker_cam.rescale", 0.25f));
		camTracker.setIterations(appSettings.get("tracker_cam.iterations", 100));
		camTracker.setClamp(appSettings.get("tracker_cam.clamp", 6.0f));
		camTracker.setTolerance(appSettings.get("tracker_cam.tolerance", 0.3f));
		camTracker.setAttempts(appSettings.get("tracker_cam.attempts", 2));
	}
	else 
	{
		camTracker.setRescale(appSettings.get("tracker_video.rescale", 1.0f));
		camTracker.setIterations(appSettings.get("tracker_video.iterations", 400));
		camTracker.setClamp(appSettings.get("tracker_video.clamp", 20.0f));
		camTracker.setTolerance(appSettings.get("tracker_video.tolerance", .01f));
		camTracker.setAttempts(appSettings.get("tracker_video.attempts", 2));
	}
	srcTracker.setup();
	srcTracker.setRescale(appSettings.get("tracker_source.rescale", 1.0f));
	srcTracker.setIterations(appSettings.get("tracker_source.iterations", 200));
	srcTracker.setClamp(appSettings.get("tracker_source.clamp", 20.0f));
	srcTracker.setTolerance(appSettings.get("tracker_source.tolerance", .01f));
	srcTracker.setAttempts(appSettings.get("tracker_source.attempts", 125.0f));

	// settings for Clone
	ofFbo::Settings settings;
	if (USECAM) 
	{
		clone.setup(cam.getWidth(), cam.getHeight());
		settings.width = cam.getWidth();
		settings.height = cam.getHeight();
	}
	else 
	{
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

	foreHeadSize = 20; // this is the height of the forehead, needs to be specified per person

	// load standard face from folder /faces.
	currentFace = 0;
	if (faces.size() != 0) {
		loadForehead(faces.getPath(currentFace));
	}
	writeStatus("App setup complete.");
}

void ofApp::update()
{
	if (USECAM) cam.update();	//update camera
	else vidPlayer.update();	// update vidplayer

	if (USECAM && cam.isFrameNew())  //check for new frame
	{                                 
		camTracker.update(toCv(cam));                          //send camera frame in Cv format
		botoxMe();

		position = camTracker.getPosition();                   //retrieve position
		scale = camTracker.getScale();                         //retrieve scale of face
		orientation = camTracker.getOrientation();             //retrieve orientation of face
		rotationMatrix = camTracker.getRotationMatrix();       //retrieve rotation
	}

	// different loop for video
	else if (vidPlayer.isFrameNew())
	{
		detectionFailed = !camTracker.update(toCv(vidPlayer));
		if (!detectionFailed) //send camera frame in Cv
		{                         
			botoxMe();
			// This causes a bug, it doesn't really work. Press 'n' to call nextFrame() manually
			// vidPlayer.nextFrame();
		}
		else 
		{
			// detectionFailed = true;
		}
		if (isRecording) 
		{
			saveFrame();
			vidPlayer.nextFrame();
			// camTracker.reset();
		}
	}
	if (vidPlayer.getIsMovieDone()) {
		writeStatus("End of video.");
		if (AUTO_EXIT) ofExit();
	}
}

void ofApp::draw()
{
	if (DEBUG_SOURCE && src.getWidth() > 0) {
		src.draw(640, 0);
	}
	if (cloneReady && cloneVisible) {
		if (!DEBUG_FOREHEAD) clone.draw(0, 0);
		else srcFbo.draw(0, 0);

		if (DEBUG_MESH) {
			camMesh.drawWireframe();

			ofPushMatrix();
			ofTranslate(640, 0);
			srcMesh.drawWireframe();
			ofPopMatrix();
		}
		if (DEBUG_LINES) {
			drawForeheadPoints(); // press 'l'
		}
	}
	else {
		if (USECAM) cam.draw(0, 0);
		else vidPlayer.draw(0, 0);
	}

	if (DEBUG_TEXT)
	{
		ofDrawBitmapString(ofToString((int)ofGetFrameRate()), 10, 20);
		ofDrawBitmapString("Clone Strength: " + ofToString(cloneStrength), 10, 40);
		ofDrawBitmapString("Forehead size: " + ofToString(foreHeadSize), 10, 60);
		ofDrawBitmapString("frame: " + ofToString(vidPlayer.getCurrentFrame()) + "/" + ofToString(vidPlayer.getTotalNumFrames()), 20, 380);
		ofDrawBitmapString("duration: " + ofToString(vidPlayer.getPosition()*vidPlayer.getDuration(), 2) + "/" + ofToString(vidPlayer.getDuration(), 2), 20, 400);
		ofDrawBitmapString("speed: " + ofToString(vidPlayer.getSpeed(), 2), 20, 420);
		ofDrawBitmapString("is recording: " + ofToString(isRecording), 20, 440);
	}

	// status text
	ofDrawBitmapString(statusText, 20, ofGetWindowHeight()-20);
}

// Function for adding mask over forehead to replace wrinkles based on neutral face
void ofApp::botoxMe() 
{
	cloneReady = camTracker.getFound();
	if (cloneReady) 
	{
		camMesh = findForehead(camTracker);             // find the forehead on the cam/vid
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
		if (USECAM) clone.update(srcFbo.getTexture(), cam.getTexture(), maskFbo.getTexture());
		else clone.update(srcFbo.getTexture(), vidPlayer.getTexture(), maskFbo.getTexture());
	}
	else 
	{
		// clone failed.. reset?
		camTracker.reset();
	}
}

void ofApp::loadForehead(string face) 
{
	src.load(face);
	if (src.getWidth() > 0) 
	{
		srcTracker.update(toCv(src));
		srcMesh = findForehead(srcTracker);
		srcPoints.resize(srcMesh.getNumVertices());
		for (int i = 0; i < srcPoints.size(); i++) 
		{
			srcPoints[i] = glm::vec2(srcMesh.getVertex(i));
		}
	}
}

void ofApp::dragEvent(ofDragInfo dragInfo) 
{
	loadForehead(dragInfo.files[0]);
}

ofPolyline ofApp::getForeheadPoly(const ofxFaceTracker& t)
{
	// retrieve all coordinates
	glm::vec2 le1 = t.getImagePoint(17);      // retrieve coordinates of left eyebrow out
	glm::vec2 le2 = t.getImagePoint(18);      // retrieve coordinates of left eyebrow
	glm::vec2 le3 = t.getImagePoint(19);      // retrieve coordinates of left eyebrow
	glm::vec2 le4 = t.getImagePoint(20);      // retrieve coordinates of left eyebrow
	glm::vec2 le5 = t.getImagePoint(21);      // retrieve coordinates of left eyebrow in
	glm::vec2 nb = t.getImagePoint(27);      // retrieve coordinates of nose bridge
	glm::vec2 re1 = t.getImagePoint(26);      // retrieve coordinates of right eyebrow out
	glm::vec2 re2 = t.getImagePoint(25);      // retrieve coordinates of right eyebrow
	glm::vec2 re3 = t.getImagePoint(24);      // retrieve coordinates of right eyebrow
	glm::vec2 re4 = t.getImagePoint(23);      // retrieve coordinates of right eyebrow
	glm::vec2 re5 = t.getImagePoint(22);      // retrieve coordinates of right eyebrow in
	glm::vec2 ls = t.getImagePoint(0);      // left side of the face
	glm::vec2 rs = t.getImagePoint(16);     // right side of the face

	// determine forehead top location.
	float foreheadTop = nb.y - (foreHeadSize*t.getScale());

	// resolution for curves from point to point
	int topResolution = 11;
	float bottomResolution = 5;

	// create the polyline.
	ofPolyline poly;
	poly.curveTo(ls.x, ls.y, 0, topResolution);									//from left side
	poly.curveTo(ls.x, ls.y - ((ls.y - foreheadTop) / 2), 0, topResolution);	//to left top side 1
	poly.curveTo(ls.x + ((nb.x - ls.x) / 2), foreheadTop, 0, topResolution);
	poly.curveTo(nb.x, foreheadTop, 0, topResolution);							//to top of the head
	poly.curveTo(rs.x - ((rs.x - nb.x) / 2), foreheadTop, 0, topResolution);
	poly.curveTo(rs.x, rs.y - ((ls.y - foreheadTop) / 2), 0, topResolution);	//to right top side
	poly.curveTo(rs.x, rs.y, 0, topResolution);		//to right side
	poly.addVertex(rs.x, rs.y);							//to right side
	poly.addVertex(re1.x, re1.y);						//to right eye
	poly.curveTo(re1.x, re1.y, 0, bottomResolution);	//to right eye
	poly.curveTo(re2.x, re2.y, 0, bottomResolution);	//to right eye
	poly.curveTo(re3.x, re3.y, 0, bottomResolution);	//to right eye
	poly.curveTo(re4.x, re4.y, 0, bottomResolution);	//to right eye
	poly.curveTo(re5.x, re5.y, 0, bottomResolution);	//to right eye
	poly.curveTo(nb.x, nb.y, 0, bottomResolution);		//to nosebridge
	poly.curveTo(le5.x, le5.y, 0, bottomResolution);	//to left eye
	poly.curveTo(le4.x, le4.y, 0, bottomResolution);	//to left eye
	poly.curveTo(le3.x, le3.y, 0, bottomResolution);	//to left eye
	poly.curveTo(le2.x, le2.y, 0, bottomResolution);	//to left eye
	poly.curveTo(le1.x, le1.y, 0, bottomResolution);	//to left eye
	poly.addVertex(le1.x, le1.y);						//to left eye
	poly.addVertex(ls.x, ls.y);							//from left side

	return poly;
}

// This function finds the forehead based on the eyebrows and forehead size.
ofMesh ofApp::findForehead(const ofxFaceTracker& t)
{
	ofPolyline foreheadPoly = getForeheadPoly(t);

	ofMesh mesh;
	mesh.setMode(OF_PRIMITIVE_TRIANGLE_STRIP);
	vector<glm::vec3> vertices = foreheadPoly.getVertices();
	for (int i = 0; i < vertices.size(); i++)
	{
		int newIndex = (i % 2) ? i/2 : (vertices.size()-1) - i/2;
		mesh.addVertex(glm::vec3(vertices[newIndex].x, vertices[newIndex].y, 0));
	}
	return mesh;
}

// same as above, but this draws the lines of the forehead found.
void ofApp::drawForeheadPoints()
{
	vector<glm::vec3> foreheadPoints = getForeheadPoly(camTracker).getVertices();
	vector<glm::vec2> srcImgPoints = srcTracker.getImagePoints();
	vector<glm::vec2> camImgPoints = camTracker.getImagePoints();
	int keypoints[] = { 17, 18, 19, 20, 21, 27, 26, 25, 24, 23, 22, 0, 16 };

	ofPushMatrix();
	for (int i = 0; i < camTracker.size(); i++)
	{
		if (std::find(std::begin(keypoints), std::end(keypoints), i) == std::end(keypoints))
			ofSetColor(ofColor::blueViolet);
		else
			ofSetColor(ofColor::orangeRed);

		ofDrawBitmapString(ofToString(i), camImgPoints[i]);
		ofDrawCircle(camImgPoints[i].x, camImgPoints[i].y, 2.0f);
	}
	ofSetColor(ofColor::white);
	for (int i = 0; i < foreheadPoints.size(); i++)
	{
		ofDrawCircle(foreheadPoints[i].x, foreheadPoints[i].y, 1.0f);
	}

	// draw imagepoints in different color
	ofTranslate(640, 0);
	for (int i = 0; i < srcTracker.size(); i++)
	{
		if (std::find(std::begin(keypoints), std::end(keypoints), i) == std::end(keypoints))
			ofSetColor(ofColor::blueViolet);
		else
			ofSetColor(ofColor::orangeRed);

		ofDrawBitmapString(ofToString(i), srcImgPoints[i]);
		ofDrawCircle(srcImgPoints[i].x, srcImgPoints[i].y, 2.0f);
	}
	ofPopMatrix();
	ofSetColor(ofColor::white);
}

void ofApp::saveFrame()
{
	if (!detectionFailed) 
	{
		ofPixels pixels;
		pixels.allocate(vidPlayer.getWidth(), vidPlayer.getHeight(), OF_IMAGE_COLOR_ALPHA);
		clone.buffer.readToPixels(pixels);
		frame.setFromPixels(pixels);
	}
	string imageFileName = ofToString(frameCounter);
	frame.save("caps/" + vidToLoad.getBaseName() + "/" + imageFileName + ".png");
	frameCounter++;
}

void ofApp::writeStatus(string status)
{
	statusText = status;
}

void ofApp::keyPressed(int key)
{
	switch (key)
	{
	case 'r':
		isRecording = !isRecording;
		writeStatus(isRecording ? "Started recording." : "Stopped recording. Output saved to " + ofDirectory("caps/" + vidToLoad.getBaseName()).getAbsolutePath());
		break;
	case 'b':
		cloneVisible = !cloneVisible;
		writeStatus(cloneVisible ? "Botox on" : "Botox off");
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
		writeStatus("Reset cam tracker.");
		break;
	case 'N':
		srcTracker.reset();
		writeStatus("Reset source tracker.");
		break;
	case 's':
		//decrease  clonestrength
		cloneStrength--;
		cloneStrength = ofClamp(cloneStrength, 0, 16);
		break;
	case 'S':
		//increase  clonestrength
		cloneStrength++;
		cloneStrength = ofClamp(cloneStrength, 0, 16);
		break;
	case 'p':
		// pause
		pauseVideo = !pauseVideo;
		vidPlayer.setPaused(pauseVideo);
		writeStatus(pauseVideo ? "Video paused." : "Video unpaused.");
		break;
	case OF_KEY_LEFT:
		vidPlayer.previousFrame();
		writeStatus("Frame " + vidPlayer.getCurrentFrame() + '/' + vidPlayer.getTotalNumFrames());
		break;
	case OF_KEY_RIGHT:
		camTracker.reset();
		vidPlayer.nextFrame();
		writeStatus("Frame " + vidPlayer.getCurrentFrame() + '/' + vidPlayer.getTotalNumFrames());
		break;
	case '0':
		vidPlayer.firstFrame();
		writeStatus("Frame " + vidPlayer.getCurrentFrame() + '/' + vidPlayer.getTotalNumFrames());
		break;
	case 'l':
		DEBUG_LINES = !DEBUG_LINES;
		break;
	case 'd':
		DEBUG_SOURCE = !DEBUG_SOURCE;
		break;
	case 'q':
		DEBUG_MESH = !DEBUG_MESH;
		break;
	case 'w':
		DEBUG_FOREHEAD = !DEBUG_FOREHEAD;
		break;
	}
}
