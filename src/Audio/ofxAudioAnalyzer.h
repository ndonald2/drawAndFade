//
//  ofxAudioAnalyzer.h
//  drawAndFade
//
//  Created by Nick Donaldson on 10/20/12.
//
//

#pragma once

#include "ofBaseApp.h"
#include "ofxFft.h"

typedef enum {
    AA_FREQ_REGION_LOW = 0,
    AA_FREQ_REGION_MID,
    AA_FREQ_REGION_HIGH
} ofxAudioAnalyzerRegion;

class ofxAudioAnalyzer : public ofBaseSoundInput{
    
public:
    
    struct Settings;
    struct FreqRegion;
    
    ofxAudioAnalyzer();
    ~ofxAudioAnalyzer();
    void setup(Settings settings = Settings());
    
    void audioIn(float * input, int bufferSize, int nChannels);
    
    void setLowMidHighRegions(FreqRegion lowRegion, FreqRegion midRegion, FreqRegion highRegion);
    void setAttackRelease(float attackInMs, float releaseInMs);
    
    void getFFTBins(vector<float> * bins);
    void getPositiveSpectralFlux(vector<float> * flux);     // postive spectral flux can be thresholded to detect transients
    
    float getSignalEnergy() { return signalEnergySmoothed; };
    float getSignalEnergyInRegion(ofxAudioAnalyzerRegion region);
    float getPSFinRegion(ofxAudioAnalyzerRegion region);
    
    struct Settings {
      
        int          inputDeviceId;
        bool         stereo;
        unsigned int sampleRate;
        unsigned int bufferSize;
        
        float        attackInMs;
        float        releaseInMs;
        
        fftWindowType       windowType;
        fftImplementation   implementation;
        
        // TODO: Overlap? FFT window size different from input buffer size?
        
        Settings();
    };
    
    struct FreqRegion {
        float lower;
        float upper;
        
        FreqRegion() { lower = 0.0f; upper = 0.0f; };
    };

private:
    
    Settings        _settings;
    
    ofSoundStream   inputStream;
    ofxFft          *fft;

    vector<float>   pcmBuffer;
    vector<float>   analyzedFFTData;
    vector<float>   storedFFTData;
    vector<float>   analyzedPSFData;
    vector<float>   storedPSFData;
    
    float           coefAttack;
    float           coefRelease;
    float           signalEnergySmoothed;
    float           regionEnergySmoothed[3];
    float           signalPSF;
    float           regionPSF[3];
    FreqRegion      _lowRegion;
    FreqRegion      _midRegion;
    FreqRegion      _highRegion;
    
    ofMutex         fftMutex;
    ofMutex         psfMutex;
};