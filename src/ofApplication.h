#pragma once

#include "ofMain.h"
#include "ofxOpenCv.h"
#include "ofxKinect.h"

#define USE_MOUSE

void ofApplicationSetAudioInputIndex(int index);

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
    
        void audioIn(float * input, int bufferSize, int nChannels);

    
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
        ofSoundStream audioInput;
        vector<float> left;
        vector<float> right;
        float smoothedVol;
    
        // kinect
        ofxKinect   kinect;
        int         kinectAngle;
    
        ofxCvGrayscaleImage grayImage; // grayscale depth image
        ofxCvGrayscaleImage grayThreshNear; // the near thresholded image
        ofxCvGrayscaleImage grayThreshFar; // the far thresholded image
        ofxCvContourFinder contourFinder;
    
        bool debugKinect;
    
        int nearThreshold;
        int farThreshold;
};
