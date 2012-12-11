
//
//  ofxAudioAnalyzer.cpp
//  drawAndFade
//
//  Created by Nick Donaldson on 10/20/12.
//
//

#include "ofxAudioAnalyzer.h"

static ofxAudioAnalyzer s_AudioAnalyzer;

ofxAudioAnalyzer & SharedAudioAnalyzer(){
    return s_AudioAnalyzer;
}

ofxAudioAnalyzer::Settings::Settings(){
    inputDeviceId = 0;
    stereo = false;
    sampleRate = 44100;
    bufferSize = 1024;
    windowType = OF_FFT_WINDOW_HAMMING;
    implementation = OF_FFT_FFTW;
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
    
    memset(signalEnergy, 0, sizeof(float)*AA_NUM_FREQ_REGIONS);
    memset(signalEnergySmoothed, 0, sizeof(float)*AA_NUM_FREQ_REGIONS);
    memset(signalPSF, 0, sizeof(float)*AA_NUM_FREQ_REGIONS);
    memset(signalPSFSmoothed, 0, sizeof(float)*AA_NUM_FREQ_REGIONS);
    kickEnergy = 0.0f;
    
    // regions
    _lowRegion.lowerFreq = 40.0f;
    _lowRegion.upperFreq = 150.0f;
    
    _midRegion.lowerFreq = 150.0f;
    _midRegion.upperFreq = 3000.0f;
    
    _highRegion.lowerFreq = 3000.0f;
    _highRegion.upperFreq = 20000.0f;
    
    // default attack release
    for (ofxAudioAnalyzerRegion region = AA_FREQ_REGION_LOW; region < AA_NUM_FREQ_REGIONS; region++){
        setAttackInRegion(1.0f, region);
        setReleaseInRegion(150.0f, region);
    }
    
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
    
    pcmMutex.lock();
    if (nChannels > 1){
        for (int i=0; i<bufferSize; i++){
            pcmBuffer[i] = input[i*2]*0.5f;
            pcmBuffer[i] += input[i*2+1]*0.5f;
        }
    }
    else{
        memcpy(&pcmBuffer[0], input, bufferSize*sizeof(float));
    }
    pcmMutex.unlock();
    
    int nBins = fft->getBinSize();
    
    fft->setSignal(pcmBuffer);
    float *fftAmp = fft->getAmplitude();
    memcpy(&analyzedFFTData[0], fftAmp, nBins*sizeof(float));
    
    // calculate psf & signal energy
    float binFlux = 0;
    float totalFlux = 0;
    float energy = 0;
    for (int i=0; i<nBins; i++){
        binFlux = MAX(0,analyzedFFTData[i] - storedFFTData[i]);
        analyzedPSFData[i] = binFlux;
        totalFlux += binFlux;
        energy += analyzedFFTData[i];
    }
    
    signalEnergy[AA_FREQ_REGION_ALL] = energy;
    signalPSF[AA_FREQ_REGION_ALL] = totalFlux;
    
    float coef = energy > signalEnergySmoothed[AA_FREQ_REGION_ALL] ? coefAttack[AA_FREQ_REGION_ALL] : coefRelease[AA_FREQ_REGION_ALL];
    float smoothedEnergy = signalEnergySmoothed[AA_FREQ_REGION_ALL]*coef + signalEnergy[AA_FREQ_REGION_ALL]*(1.0f-coef);
    signalEnergySmoothed[AA_FREQ_REGION_ALL] = smoothedEnergy;
    
    coef = totalFlux > signalPSFSmoothed[AA_FREQ_REGION_ALL] ? coefAttack[AA_FREQ_REGION_ALL] : coefRelease[AA_FREQ_REGION_ALL];
    float smoothedFlux = signalPSFSmoothed[AA_FREQ_REGION_ALL]*coef + signalPSF[AA_FREQ_REGION_ALL]*(1.0f-coef);
    signalPSFSmoothed[AA_FREQ_REGION_ALL] = smoothedFlux;
    
    // get region fft & PSF
    FreqRegion fRegion;
    for (unsigned int region = AA_FREQ_REGION_LOW; region <= AA_FREQ_REGION_HIGH; region++){
        switch (region) {
            case AA_FREQ_REGION_LOW:
                fRegion = _lowRegion;
                break;
                
            case AA_FREQ_REGION_MID:
                fRegion = _midRegion;
                break;
                
            case AA_FREQ_REGION_HIGH:
                fRegion = _highRegion;
                break;
                
            default:
                continue;
        }
        
        unsigned int lowerBin = binForFrequency(fRegion.lowerFreq);
        unsigned int upperBin = binForFrequency(fRegion.upperFreq);
        
        float trEnergy = 0.0f;
        float trPSF = 0.0f;
        if (upperBin >= lowerBin){
            unsigned int b = lowerBin;
            do {
                trEnergy += analyzedFFTData[b];
                trPSF += analyzedPSFData[b];
            } while (++b <= upperBin);
        }
        
        signalPSF[region] = trPSF;
        signalEnergy[region] = trEnergy;
        
        float coef = trPSF >= signalPSFSmoothed[region] ? coefAttack[region] : coefRelease[region];
        float psfSmoothed = signalPSFSmoothed[region]*coef + trPSF*(1.0f-coef);
        signalPSFSmoothed[region] = psfSmoothed;
        
        coef = trEnergy >= signalEnergySmoothed[region] ? coefAttack[region] : coefRelease[region];
        float enSmoothed = signalEnergySmoothed[region]*coef + trEnergy*(1.0f-coef);
        signalEnergySmoothed[region] = enSmoothed;
    }

    float newKickEnergy = signalEnergySmoothed[AA_FREQ_REGION_LOW] * signalPSFSmoothed[AA_FREQ_REGION_LOW];
    float kickCoef = newKickEnergy > kickEnergy ? coefAttack[AA_FREQ_REGION_LOW] : coefRelease[AA_FREQ_REGION_LOW];
    kickEnergy = kickEnergy*coef + newKickEnergy*(1.0f-coef);
    
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

void ofxAudioAnalyzer::setAttackInRegion(int attackInMS, ofxAudioAnalyzerRegion region)
{
    if (region >= AA_NUM_FREQ_REGIONS || region < 0)
    {
        ofLog(OF_LOG_ERROR, "Invalid frequency region");
        return;
    }
    
    float newCoef = 1.0f - (1.0f/(attackInMS*0.001f*_settings.sampleRate/_settings.bufferSize));
    coefAttack[region] = CLAMP(newCoef, 0.0f, 0.99999999f);
}

void ofxAudioAnalyzer::setReleaseInRegion(int releaseInMS, ofxAudioAnalyzerRegion region)
{
    if (region >= AA_NUM_FREQ_REGIONS || region < 0)
    {
        ofLog(OF_LOG_ERROR, "Invalid frequency region");
        return;
    }
    
    float newCoef = 1.0f - (1.0f/(releaseInMS*0.001f*_settings.sampleRate/_settings.bufferSize));
    coefRelease[region] = CLAMP(newCoef, 0.0f, 0.99999999f);
}

vector<float> ofxAudioAnalyzer::getFFTBins()
{
    vector<float> bins;
    fftMutex.lock();
    bins = storedFFTData;
    fftMutex.unlock();
    return bins;
}

vector<float> ofxAudioAnalyzer::getPSFData()
{
    vector<float> flux;
    psfMutex.lock();
    flux = storedPSFData;
    psfMutex.unlock();
    return flux;
}

vector<float> ofxAudioAnalyzer::getPCMData()
{
    vector<float> pcm;
    pcmMutex.lock();
    pcm = pcmBuffer;
    pcmMutex.unlock();
    return pcm;
}

float ofxAudioAnalyzer::getSignalEnergy(bool smoothed)
{
    return smoothed ? signalEnergySmoothed[AA_FREQ_REGION_ALL] : signalEnergySmoothed[AA_FREQ_REGION_ALL];
}

float ofxAudioAnalyzer::getSignalEnergyInRegion(ofxAudioAnalyzerRegion region, bool smoothed)
{
    if (region < 0 || region > 3)
        return 0.0f;
    
    return smoothed ? signalEnergySmoothed[region] : signalEnergy[region];
}

float ofxAudioAnalyzer::getTotalPSF(bool smoothed)
{
    return smoothed ? signalPSFSmoothed[AA_FREQ_REGION_ALL] : signalPSF[AA_FREQ_REGION_ALL];
}

float ofxAudioAnalyzer::getPSFinRegion(ofxAudioAnalyzerRegion region, bool smoothed)
{
    
    if (region < 0 || region >= AA_NUM_FREQ_REGIONS)
        return 0.0f;
    
    return smoothed ? signalPSFSmoothed[region] : signalPSF[region];
}

unsigned int ofxAudioAnalyzer::binForFrequency(float freqInHz)
{
    float nyquist = _settings.sampleRate/2;

    if (freqInHz <= 0 || !fft)
    {
        return 0;
    }
    else if (freqInHz >= nyquist)
    {
        return fft->getBinSize();
    }
    else{
        return MIN((unsigned int)roundf(freqInHz*fft->getBinSize()/nyquist), fft->getBinSize()-1);
    }
}
