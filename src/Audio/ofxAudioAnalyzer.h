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

#define AA_NUM_FREQ_REGIONS 4
typedef enum {
    AA_FREQ_REGION_LOW = 0,
    AA_FREQ_REGION_MID,
    AA_FREQ_REGION_HIGH,
    AA_FREQ_REGION_ALL
} ofxAudioAnalyzerRegion;

class ofxAudioAnalyzer;
extern ofxAudioAnalyzer & SharedAudioAnalyzer();

class ofxAudioAnalyzer : public ofBaseSoundInput{
    
public:
    
    struct Settings;
    struct FreqRegion;
    
    ofxAudioAnalyzer();
    ~ofxAudioAnalyzer();
    void setup(Settings settings = Settings());
    
    void audioIn(float * input, int bufferSize, int nChannels);
    
    void setLowMidHighRegions(FreqRegion lowRegion, FreqRegion midRegion, FreqRegion highRegion);
    void setAttackInRegion(int attackInMS, ofxAudioAnalyzerRegion region);
    void setReleaseInRegion(int releaseInMS, ofxAudioAnalyzerRegion region);
    
    vector<float> getFFTBins();
    vector<float> getPSFData();     // postive spectral flux can be thresholded to detect transients
    vector<float> getPCMData();
    float getSignalEnergy(bool smoothed = true);
    float getSignalEnergyInRegion(ofxAudioAnalyzerRegion region, bool smoothed = true);
    float getTotalPSF(bool smoothed = true);
    float getPSFinRegion(ofxAudioAnalyzerRegion region, bool smoothed = true);
    inline float getKickEnergy() { return kickEnergy; };
    
    struct Settings {
      
        int          inputDeviceId;
        bool         stereo;
        unsigned int sampleRate;
        unsigned int bufferSize;
        
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
    
    float           coefAttack[AA_NUM_FREQ_REGIONS];
    float           coefRelease[AA_NUM_FREQ_REGIONS];
    float           signalEnergy[AA_NUM_FREQ_REGIONS];
    float           signalEnergySmoothed[AA_NUM_FREQ_REGIONS];
    float           signalPSF[AA_NUM_FREQ_REGIONS];
    float           signalPSFSmoothed[AA_NUM_FREQ_REGIONS];
    float           kickEnergy;
    
    FreqRegion      _lowRegion;
    FreqRegion      _midRegion;
    FreqRegion      _highRegion;
    
    ofMutex         pcmMutex;
    ofMutex         fftMutex;
    ofMutex         psfMutex;
    
    // helpers
    unsigned int binForFrequency(float freqInHz);
};