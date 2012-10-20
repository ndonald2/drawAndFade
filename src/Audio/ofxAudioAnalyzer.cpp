
//
//  ofxAudioAnalyzer.cpp
//  drawAndFade
//
//  Created by Nick Donaldson on 10/20/12.
//
//

#include "ofxAudioAnalyzer.h"


ofxAudioAnalyzer::Settings::Settings(){
    inputDeviceId = 0;
    stereo = false;
    sampleRate = 44100;
    bufferSize = 1024;
    windowType = OF_FFT_WINDOW_HAMMING;
    implementation = OF_FFT_FFTW;
    
    attackInMs = 1.0f;
    releaseInMs = 100.0f;
}

ofxAudioAnalyzer::ofxAudioAnalyzer()
{
    fft = NULL;
}

void ofxAudioAnalyzer::setup(Settings settings)
{
    _settings = settings;
    
    // setup FFT
    fft = ofxFft::create(settings.bufferSize, settings.windowType, settings.implementation);
    
    // vectors

    pcmBuffer.resize(settings.bufferSize);
    analyzedFFTData.resize(fft->getBinSize());
    analyzedFFTData.assign(fft->getBinSize(), 0);
    storedFFTData.resize(fft->getBinSize());
    storedFFTData.assign(fft->getBinSize(), 0);
    analyzedPSFData.resize(fft->getBinSize());
    analyzedPSFData.assign(fft->getBinSize(), 0);
    storedPSFData.resize(fft->getBinSize());
    storedPSFData.assign(fft->getBinSize(), 0);
    
    signalEnergy = 0.0f;
    signalEnergySmoothed = 0.0f;
    memset(regionEnergy, 0, sizeof(float)*3);
    memset(regionEnergySmoothed, 0, sizeof(float)*3);
    signalPSF = 0.0f;
    signalPSFSmoothed = 0.0f;
    memset(regionPSF, 0, sizeof(float)*3);
    memset(regionPSFSmoothed, 0, sizeof(float)*3);
    
    setAttackRelease(settings.attackInMs, settings.releaseInMs);
    
    // setup audio input stream
    inputStream.setDeviceID(settings.inputDeviceId);
    inputStream.setInput(this);
    inputStream.setup(0, settings.stereo ? 2 : 1, settings.sampleRate, settings.bufferSize, 4);
}

ofxAudioAnalyzer::~ofxAudioAnalyzer()
{
    inputStream.stop();
    if (fft){
        delete fft;
    }
}

void ofxAudioAnalyzer::audioIn(float *input, int bufferSize, int nChannels)
{
    if (!input) return;
    
    if (nChannels > 1){
        for (int i=0; i<bufferSize; i++){
            pcmBuffer[i] = input[i*2]*0.5f;
            pcmBuffer[i] += input[i*2+1]*0.5f;
        }
    }
    else{
        memcpy(&pcmBuffer[0], input, bufferSize*sizeof(float));
    }
    
    fft->setSignal(pcmBuffer);
    float *fftAmp = fft->getAmplitude();
    memcpy(&analyzedFFTData[0], fftAmp, fft->getBinSize()*sizeof(float));
    
    // calculate psf & signal energy
    float binFlux = 0;
    float totalFlux = 0;
    float energy = 0;
    for (int i=0; i<fft->getBinSize(); i++){
        binFlux = MAX(0,analyzedFFTData[i] - storedFFTData[i]);
        analyzedPSFData[i] = binFlux;
        totalFlux += binFlux;
        energy += analyzedFFTData[i];
    }
    
    signalEnergy = energy;
    signalPSF = totalFlux;
    
    float coef = energy > signalEnergySmoothed ? coefAttack : coefRelease;
    float smoothedEnergy = signalEnergySmoothed*coef + energy*(1.0f-coef);
    signalEnergySmoothed = smoothedEnergy;
    
    coef = totalFlux > signalPSFSmoothed ? coefAttack : coefRelease;
    float smoothedFlux = signalPSFSmoothed*coef + totalFlux*(1.0f-coef);
    signalPSFSmoothed = smoothedFlux;
    
    // thread-safe assignments
    fftMutex.lock();
    storedFFTData = analyzedFFTData;
    fftMutex.unlock();
    
    psfMutex.lock();
    storedPSFData = analyzedFFTData;
    psfMutex.unlock();
}

void ofxAudioAnalyzer::setLowMidHighRegions(FreqRegion lowRegion, FreqRegion midRegion, FreqRegion highRegion)
{
    _lowRegion = lowRegion;
    _midRegion = midRegion;
    _highRegion = highRegion;
}

void ofxAudioAnalyzer::setAttackRelease(float attackInMs, float releaseInMs)
{
    _settings.attackInMs = attackInMs;
    _settings.releaseInMs = releaseInMs;
    coefAttack = 1.0f - (1.0f/(attackInMs*0.001f*_settings.sampleRate/_settings.bufferSize));
    coefRelease = 1.0f - (1.0f/(releaseInMs*0.001f*_settings.sampleRate/_settings.bufferSize));
    
    coefAttack = CLAMP(coefAttack, 0.0f, 0.9999f);
    coefRelease = CLAMP(coefRelease, 0.0f, 0.9999f);
}

void ofxAudioAnalyzer::getFFTBins(vector<float> * bins)
{
    if (bins != NULL){
        fftMutex.lock();
        *bins = storedFFTData;
        fftMutex.unlock();
    }
}

void ofxAudioAnalyzer::getPSFData(vector<float> * flux)
{
    if (flux != NULL){
        psfMutex.lock();
        *flux = storedPSFData;
        psfMutex.unlock();
    }
}

float ofxAudioAnalyzer::getSignalEnergy(bool smoothed)
{
    return smoothed ? signalEnergySmoothed : signalEnergySmoothed;
}

float ofxAudioAnalyzer::getSignalEnergyInRegion(ofxAudioAnalyzerRegion region, bool smoothed)
{
    if (region < 0 || region > 3) return 0.0f;
    
    return smoothed ? regionEnergySmoothed[region] : regionEnergy[region];
}

float ofxAudioAnalyzer::getTotalPSF(bool smoothed)
{
    return smoothed ? signalPSFSmoothed : signalPSF;
}

float ofxAudioAnalyzer::getPSFinRegion(ofxAudioAnalyzerRegion region, bool smoothed)
{
    
}