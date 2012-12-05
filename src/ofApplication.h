#pragma once

#include "ofMain.h"
#include "ofxMidi.h"
#include "ofxOsc.h"
#include "ofxOpenNI.h"
#include "ofxHardwareDriver.h"
#include "ofxOpenCv.h"
#include "ofxAudioAnalyzer.h"
#include "ofxHandPhysics.h"
#include "ofxNDGraphicsUtils.h"
#include <map>

// ================================
//      Compile-time options
// ================================

// Uncomment to use user tracking instead of hand tracking.
// Hand tracking is faster and more accurate, but loses positions occasionally.
#define USE_USER_TRACKING

extern void ofApplicationSetAudioInputDeviceId(int deviceId);
extern void ofApplicationSetMidiInputDeviceId(int deviceId);
extern void ofApplicationSetOSCListenPort(int listenPort);

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
    
    
    private:

        // osc events
        void processOscMessages();
        void handleTouchPadMessage(ofxOscMessage &m);
    
        // drawing
        void beginTrails();
        void endTrails();
        
        void updateUserOutline();
        
        void drawTrails();
        void drawPoiSprites();
        void drawHandSprites();
        void drawUserOutline();
        void drawTouches();
    
        // openGL
        ofFbo           mainFbo;
        ofFbo           trailsFbo;
        ofFbo           userFbo;
    
        ofShader        trailsShader;
        ofShader        gaussianBlurShader;
        ofShader        userMaskShader;
    
        ofPolyline          touchLine;
        map<int,ofVec2f>    touchMap;
    
        // midi
        ofxMidiIn       midiIn;
    
        // osc
        ofxOscReceiver  oscIn;
    
        // audio
        ofxAudioAnalyzer            audioAnalyzer;
        float                       audioSensitivity;
        float                       audioLowEnergy;
        float                       audioMidEnergy;
        float                       audioHiEnergy;
        float                       audioHiPSF;
    
        // kinect
#ifdef USE_KINECT
        ofxOpenNI                   kinectOpenNI;
        ofxHardwareDriver           kinectDriver;
        ofxHandPhysicsManager *     handPhysics;
        int                         kinectAngle;
#endif
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
    
        // FREEZE FRAME
        float       strobeIntervalMs;
        float       strobeLastDrawTime;
    
        // CIRCULAR GRADIENT + BACKGROUND
        float       bgBrightnessFade;
        float       bgSpotRadius;
    
        // TRAILS
        ofPoint     trailVelocity;
        ofPoint     trailAnchor;
        float       trailZoom;         // percent increase/decrease per second
        float       trailColorDecay;
        float       trailAlphaDecay;
        float       trailMinAlpha;
    
        // USER OUTLINE
        ofxNDHSBColor   userOutlineColorHSB;
        float           userShapeScaleFactor;
    
        // POI
        ofxNDHSBColor   poiSpriteColorHSB;
        float           poiMaxScaleFactor;
    
        // HANDS
        ofxNDHSBColor   handsColorHSB;

    
};
