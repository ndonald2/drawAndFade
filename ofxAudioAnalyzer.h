//
//  ofxAudioAnalyzer.h
//  drawAndFade
//
//  Created by Nick Donaldson on 10/20/12.
//
//

#pragma once

#include "ofMain.h"

class ofxAudioAnalyzer : public ofBaseApp {
    
public:
    
    ofxAudioAnalyzer();
    ~ofxAudioAnalyzer();
    
    void audioIn(float * input, int bufferSize, int nChannels);
    
    vector<float> & getFFTBins();
    

private:

    vector<float> analyzedFFTBins;
    vector<float> storedFFTBins;
    vector<float> positiveFlux;
    
};