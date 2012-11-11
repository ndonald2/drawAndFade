#pragma once

#include "ofMain.h"
#include "ofxOpenNI.h"
#include "ofxHardwareDriver.h"
#include "ofxOpenCv.h"
#include "ofxAudioAnalyzer.h"
#include "ofxHandPhysics.h"

// ================================
//      Compile-time options
// ================================

// Comment out for no-kinect debug mode (most likely out-of-date)
#define USE_KINECT

// Uncomment to use user tracking instead of hand tracking.
//#define USE_USER_TRACKING


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
        void beginTrails();
        void endTrails();
        void drawTrails();
        void drawPoiSprites();
        void drawHandSprites();
        void drawUserOutline();
    
    private:
    
        // GL
        ofFbo           mainFbo;
        ofFbo           trailsFbo;
        ofShader        trailsShader;
        ofShader        userOutlineShader;
    
        // renderer state
        float   elapsedPhase;
    
        // blur parameters
        ofPoint trailVelocity;
        ofPoint trailScale;
    
        // audio
        ofxAudioAnalyzer audioAnalyzer;
    
        // kinect
        ofxOpenNI                   kinectOpenNI;
        ofxHardwareDriver           kinectDriver;
        ofxHandPhysicsManager *     handPhysics;
        int                         kinectAngle;
        float                       depthThresh;
    
        // animation options
        bool        debugMode;
        bool        showTrails;
        float       trailColorDecay;
        float       trailAlphaDecay;
        float       trailMinAlpha;
    
        // colors
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
