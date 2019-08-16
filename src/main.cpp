#include "ofApp.h"
#include "ofAppGlutWindow.h"

int main(int argc, char *argv[])
{
	ofAppGLFWWindow window;

	ofSetupOpenGL(&window, 1280, 800, OF_WINDOW);

	ofApp* app = new ofApp();
	app->args = vector<string>(argv, argv + argc);

	ofRunApp(app);
}
