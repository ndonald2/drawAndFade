#pragma once

#include "ofMain.h"
#include "ofxOsc.h"
#include "ofxOpenNI.h"
#include "ofxHardwareDriver.h"
#include "ofxOpenCv.h"
#include "ofxAudioAnalyzer.h"
#include "ofxNDGraphicsUtils.h"
#include <map>

// ================================
//      Compile-time options
// ================================

// Uncomment to use user tracking instead of hand tracking.
// Hand tracking is faster and more accurate, but loses positions occasionally.
#define USE_USER_TRACKING

extern void ofApplicationSetAudioInputDeviceId(int deviceId);
extern void ofApplicationSetOSCListenPort(int listenPort);

class ofApplication : public ofBaseApp {
	public:
    
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
    
    private:

        // osc events
        void processOscMessages();
    
        // drawing
        void beginTrails();
        void endTrails();
        
        void updateUserOutline();
        void drawShapeSkeletons();
        void drawCirclesForLimb(ofxOpenNILimb & limb);
    
        void drawTrails();
        void drawUserOutline();
    
        // openGL
        ofFbo           mainFbo;
        ofFbo           trailsFbo;
        ofFbo           userFbo;
    
        ofShader        trailsShader;
        ofShader        gaussianBlurShader;
        ofShader        userMaskShader;
    
        ofVec3f         screenNormScale;
    
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
        int                         kinectAngle;
#endif
        // renderer state
        float       elapsedPhase;
        bool        debugMode;
    
        // assets and components
        ofImage     paperImage;
        ofPoint     paperInset;
    
    
        // ----- animation options ------
    
        // FLAGS
        bool        bDrawUserOutline;
        bool        bTrailUserOutline;
    
        // FREEZE FRAME
        float       strobeIntervalMs;
        float       strobeLastDrawTime;
    
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
    
};
