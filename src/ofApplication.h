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

// Comment out for no-kinect debug mode
#define USE_KINECT

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
    
        void updateUserOutline();
    
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
        float       elapsedPhase;
        bool        debugMode;

    
        // ----- animation options ------
    
        // FLAGS
        bool        bDrawUserOutline;
        bool        bTrailUserOutline;
        bool        bDrawHands;
        bool        bTrailHands;
        bool        bDrawPoi;
        bool        bTrailPoi;
    
        // CIRCULAR GRADIENT + BACKGROUND
        ofColor     bgColor;
        ofColor     gradCircleColor;
        ofPoint     gradCircleCenter;
        float       gradCircleRadius;
    
        // TRAILS
        ofPoint     trailVelocity;
        ofPoint     trailScale;         // percent increase/decrease per second
        ofPoint     trailScaleAnchor;
        float       trailColorDecay;
        float       trailAlphaDecay;
        float       trailMinAlpha;
    
        // USER OUTLINE
        ofColor     userOutlineColor;
        float       userShapeScaleFactor;
    
        // POI
        ofColor     poiSpriteColor;
        float       poiMaxScaleFactor;
    
        // HANDS
        ofColor     handsColor;
    
};
