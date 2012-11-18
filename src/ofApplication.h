#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxOpenNI.h"
#include "ofxHardwareDriver.h"
#include "ofxOpenCv.h"
#include "ofxAudioAnalyzer.h"
#include "ofxHandPhysics.h"

// ================================
//      Compile-time options
// ================================

// Comment out for no-kinect debug mode (most likely out-of-date)
//#define USE_KINECT

// Uncomment to use user tracking instead of hand tracking.
// Hand tracking is much faster/more accurate, but loses positions occasionally.
#define USE_USER_TRACKING


void ofApplicationSetAudioInputDeviceId(int deviceId);

class ofApplication : public ofBaseApp, public ofxMidiListener {
	public:
    
        ofApplication();
        ~ofApplication();
    
		void setup();
		void update();
		void draw();
		
        // default events
		void keyPressed(int key);
		void keyReleased(int key);
		void mouseMoved(int x, int y);
		void mouseDragged(int x, int y, int button);
		void mousePressed(int x, int y, int button);
		void mouseReleased(int x, int y, int button);
		void windowResized(int w, int h);
		void dragEvent(ofDragInfo dragInfo);
		void gotMessage(ofMessage msg);
    
        // midi events
        void newMidiMessage(ofxMidiMessage& msg);
    
        // drawing
        void beginTrails();
        void endTrails();
    
        void blurUserOutline();
    
        void drawTrails();
        void drawPoiSprites();
        void drawHandSprites();
        void drawUserOutline();
    
    private:
        
        // openGL
        ofFbo           mainFbo;
        ofFbo           trailsFbo;
        ofFbo           userFbo;
    
        ofShader        trailsShader;
        ofShader        gaussianBlurShader;
        ofShader        userMaskShader;

    
        // midi
        ofxMidiIn       midiIn;
    
    
        // audio
        ofxAudioAnalyzer            audioAnalyzer;
        float                       audioSensitivity;
    
        // kinect
        ofxOpenNI                   kinectOpenNI;
        ofxHardwareDriver           kinectDriver;
        ofxHandPhysicsManager *     handPhysics;
        int                         kinectAngle;
    
        // renderer state
        float   elapsedPhase;
    
        // animation options
        bool        debugMode;
        bool        showTrails;
    
        ofPoint     trailVelocity;
        ofPoint     trailScale;         // percent increase/decrease per second
        ofPoint     trailScaleAnchor;
        float       trailColorDecay;
        float       trailAlphaDecay;
        float       trailMinAlpha;
    
        float       poiMaxScaleFactor;
        float       userShapeScaleFactor;
    
        // colors
        ofColor     handsColor;
        ofColor     poiSpriteColor;
        ofColor     bgColor;
        ofColor     bgHighlightColor;
    
#ifdef USE_MOUSE
    ofPoint lastEndPoint;
    ofPoint lastMousePoint;
    float   mouseVelocity;
#endif
    
};
