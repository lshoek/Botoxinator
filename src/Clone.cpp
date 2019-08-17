#include "Clone.h"

void Clone::setup(int width, int height) {
    ofFbo::Settings settings;
    settings.width = width;
    settings.height = height;
    
    buffer.allocate(settings);
    srcBlur.allocate(settings);
    dstBlur.allocate(settings);
    
	maskBlurShader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shaders/maskblur.frag");
	cloneShader.setupShaderFromFile(GL_FRAGMENT_SHADER, "shaders/clone.frag");
    maskBlurShader.linkProgram();
    cloneShader.linkProgram();
    
    strength = 0;
}

void Clone::maskedBlur(ofTexture& tex, ofTexture& mask, ofFbo& result) {
    int k = strength;
    
    buffer.begin();
    maskBlurShader.begin();
    maskBlurShader.setUniformTexture("tex", tex, 1);
    maskBlurShader.setUniformTexture("mask", mask, 2);
    maskBlurShader.setUniform2f("direction", 1, 0);
    maskBlurShader.setUniform1i("k", k);
    tex.draw(0, 0);
    maskBlurShader.end();
    buffer.end();
    
    result.begin();
    maskBlurShader.begin();
    maskBlurShader.setUniformTexture("tex", buffer, 1);
    maskBlurShader.setUniformTexture("mask", mask, 2);
    maskBlurShader.setUniform2f("direction", 0, 1);
    maskBlurShader.setUniform1i("k", k);
    buffer.draw(0, 0);
    maskBlurShader.end();
    result.end();
}

void Clone::setStrength(int strength) {
    this->strength = strength;
}

void Clone::update(ofTexture& src, ofTexture& dst, ofTexture& mask) {
    maskedBlur(src, mask, srcBlur);
    maskedBlur(dst, mask, dstBlur);
    
    buffer.begin();
    ofPushStyle();
    ofEnableAlphaBlending();
    dst.draw(0, 0);
    cloneShader.begin();
    cloneShader.setUniformTexture("src", src, 1);
    cloneShader.setUniformTexture("srcBlur", srcBlur, 2);
    cloneShader.setUniformTexture("dstBlur", dstBlur, 3);
    dst.draw(0, 0);
    cloneShader.end();
    ofDisableAlphaBlending();
    ofPopStyle();
    buffer.end();
}

void Clone::draw(float x, float y) {
    buffer.draw(x, y);
}

void Clone::draw(float x, float y, float width, float height) {
	buffer.draw(x, y, width, height);
}
