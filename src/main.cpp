#include "ofApp.h"
#include "ofAppGlutWindow.h"

int main() 
{
	ofAppGLFWWindow window;
	ofSetupOpenGL(&window, 1280, 800, OF_WINDOW);

	ofRunApp(new ofApp());
}
