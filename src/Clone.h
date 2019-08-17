#pragma once

#include "ofMain.h"

class Clone {
public:
    void setup(int width, int height);
    void setStrength(int strength);
    void update(ofTexture& src, ofTexture& dst, ofTexture& mask);
    void draw(float x, float y);
	void draw(float x, float y, float width, float height);
    ofFbo buffer;
    
protected:
    void maskedBlur(ofTexture& tex, ofTexture& mask, ofFbo& result);
    ofFbo srcBlur, dstBlur;
    ofShader maskBlurShader, cloneShader;
    int strength;
};