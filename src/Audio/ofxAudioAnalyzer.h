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
    void getPSFData(vector<float> * flux);     // postive spectral flux can be thresholded to detect transients
    float getSignalEnergy(bool smoothed = true);
    float getSignalEnergyInRegion(ofxAudioAnalyzerRegion region, bool smoothed = true);
    float getTotalPSF(bool smoothed = true);
    float getPSFinRegion(ofxAudioAnalyzerRegion region, bool smoothed = true);
    
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
        
        float lowerFreq;
        float upperFreq;
        
        // TODO
        //float attackInMs;
        //float releaseInMs;
        
        FreqRegion() {
            lowerFreq = 0.0f;
            upperFreq = 0.0f;
        };
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
    float           signalEnergy;
    float           regionEnergy[3];
    float           signalEnergySmoothed;
    float           regionEnergySmoothed[3];
    float           signalPSF;
    float           regionPSF[3];
    float           signalPSFSmoothed;
    float           regionPSFSmoothed[3];
    FreqRegion      _lowRegion;
    FreqRegion      _midRegion;
    FreqRegion      _highRegion;
    
    ofMutex         fftMutex;
    ofMutex         psfMutex;
    
    // helpers
    unsigned int binForFrequency(float freqInHz);
};