#pragma once

#include "ofMain.h"
#include "ofxOpenNI.h"
#include "ofxOpenCv.h"
#include "ofxAudioAnalyzer.h"
#include "ofxHandPhysics.h"

#define USE_KINECT

void ofApplicationSetAudioInputDeviceId(int deviceId);

class ofApplication : public ofBaseApp{
	public:
    
        ofApplication();
        ~ofApplication();
    
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
    
        // drawing
        void drawHandSprites();
        void drawAudioBlobs();
        void drawBillboardRect(int x, int y, int w, int h);
    
    private:
    
        ofFbo           mainFbo;
        ofShader        blurShader;
    
        // state
        float   elapsedPhase;
    
        // blur parameters
        ofPoint blurDirection;
        ofPoint blurVelocity;
    
        // audio
        ofxAudioAnalyzer audioAnalyzer;
    
        // kinect
        ofxOpenNI                   kinectOpenNI;
        ofxHandPhysicsManager *     handPhysics;
    
        // animation options
        bool        debugMode;
        bool        showTrails;
        ofColor     audioBlobColor;
        ofColor     spriteColor;
        ofColor     bgColor;
        ofColor     bgHighlightColor;
    
#ifdef USE_MOUSE
    ofPoint lastEndPoint;
    ofPoint lastMousePoint;
    float   mouseVelocity;
#endif
    
};
