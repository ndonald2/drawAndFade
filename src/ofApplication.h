#pragma once

#include "ofMain.h"
#include "ofxOpenNI.h"
#include "ofxOpenCv.h"
#include "ofxAudioAnalyzer.h"
#include "ofxHandPhysics.h"

//#define USE_MOUSE

void ofApplicationSetAudioInputDeviceId(int deviceId);

class ofApplication : public ofBaseApp{
	public:
		void setup();
		void update();
		void draw();
		
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
    private:
    
        ofFbo           mainFbo;
        ofShader        blurShader;
    
        ofPoint lastEndPoint;
        ofPoint lastMousePoint;
        float   mouseVelocity;
    
        // blur parameters
        ofPoint blurDirection;
        ofPoint blurVelocity;
    
        // audio
        ofxAudioAnalyzer audioAnalyzer;
    
        // kinect
        ofxOpenNI                   kinectOpenNI;
        ofxHandPhysicsManager *     handPhysics;
};
