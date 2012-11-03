#include "ofApplication.h"

#define MAX_BLOTCH_RADIUS    50.0f

static int inputDeviceId = 0;

void ofApplicationSetAudioInputDeviceId(int deviceId){
    inputDeviceId = deviceId;
}

//--------------------------------------------------------------

ofApplication::ofApplication()
{
    handPhysics = NULL;
}

ofApplication::~ofApplication()
{
    if (handPhysics){
        delete handPhysics;
        handPhysics = NULL;
    }
    
    // prevents crashing on exit (sometimes)
    kinectOpenNI.stop();
    kinectOpenNI.waitForThread();
}

void ofApplication::setup(){
    
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    ofEnableAlphaBlending();
    ofEnableSmoothing();
    
    // setup animation parameters
    debugMode = false;
    showTrails = true;
    audioBlobColor = ofColor(180,180,180);
    
#ifdef USE_MOUSE
    mouseVelocity = 0.0f;
#endif
    
    ofFbo::Settings fboSettings;
    fboSettings.width = ofGetWidth();
    fboSettings.height = ofGetHeight();
    fboSettings.useDepth = true;
    fboSettings.useStencil = false;
    fboSettings.depthStencilAsTexture = false;
    fboSettings.numColorbuffers = 2;
    fboSettings.internalformat = GL_RGBA32F_ARB;
        
    mainFbo.allocate(fboSettings);
    mainFbo.begin();
    ofClear(255,255,255,10);
    mainFbo.setActiveDrawBuffer(1);
    ofClear(255,255,255,10);
    mainFbo.end();
    
    blurShader.load("shaders/blur.vert", "shaders/blur.frag");
    blurDirection = ofPoint(0,0);
    blurVelocity = 10;
    
    // audio setup
    ofxAudioAnalyzer::Settings audioSettings;
    audioSettings.stereo = true;
    audioSettings.inputDeviceId = inputDeviceId;
    audioSettings.bufferSize = 512;
    audioAnalyzer.setup(audioSettings);
    
    // kinect setup
    kinectOpenNI.setup();
    kinectOpenNI.addImageGenerator();
    kinectOpenNI.addDepthGenerator();
    kinectOpenNI.setRegister(true);
    kinectOpenNI.setMirror(true);
    kinectOpenNI.setSafeThreading(true);
    
    // setup the hand generator
    kinectOpenNI.addHandsGenerator();
    kinectOpenNI.addHandFocusGesture("RaiseHand");
    kinectOpenNI.addHandFocusGesture("MovingHand");
    kinectOpenNI.setMaxNumHands(2);
    
    kinectOpenNI.start();
    
    handPhysics = new ofxHandPhysicsManager(kinectOpenNI);
    handPhysics->restDistance = 40.0f;
    handPhysics->friction = 0.03f;
    handPhysics->gravity = ofVec2f(0,800.0f);
    handPhysics->physicsEnabled = true;
}

//--------------------------------------------------------------
void ofApplication::update(){
    
    elapsedPhase = 2.0*M_PI*ofGetElapsedTimef();

    // draw to FBO
    
    glDisable(GL_DEPTH_TEST);
    mainFbo.begin();
    ofFill();
    
    if (showTrails){
        if (ofGetFrameNum() % 180 == 0)
        {
            blurVelocity = ofPoint(1.0f,1.0f)*ofRandom(100.0f, 300.0f);
            blurDirection = ofPoint(1.0f,1.0f)*ofRandom(-0.2f, 0.2f);
        }
        
        ofPoint scaledBlurVelocity = blurVelocity/10000.0f;
        ofPoint scaledBlurDirection = (ofPoint(0.5,0.5) + blurDirection) * scaledBlurVelocity;
        
        ofTexture & fadingTex = mainFbo.getTextureReference(1);

        mainFbo.setActiveDrawBuffer(0);
        ofSetColor(ofColor(255,255,255));
        
        ofPushMatrix();
        ofScale(1.0f + scaledBlurVelocity.x, 1.0f + scaledBlurVelocity.y);
        
        // re center
        ofPoint translation = -(ofPoint(ofGetWidth(), ofGetHeight())*scaledBlurDirection);
        ofTranslate(translation);
        fadingTex.draw(0,0);
        ofPopMatrix();
    }
    else{
        mainFbo.setActiveDrawBuffer(0);
        ofClear(255, 255, 255, 0);
    }

#ifdef USE_MOUSE
    if (ofGetMousePressed()){
        
        float highEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH)*100.0f;

        ofPoint mousePoint = ofPoint(ofGetMouseX(), ofGetMouseY());
        mouseVelocity = fabs(mousePoint.distance(lastMousePoint))*100.0f/ofGetFrameRate();
        lastMousePoint = mousePoint;
        
        float saturation = ofMap(mouseVelocity, 0.0f, 100.0f, 255.0f, 180.0f, true);
        float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
        float radius = ofMap(highEnergy, 0.2f, 2.0f, 0.0f, (float)ofGetWidth()*MAX_BLOTCH_RADIUS_FACTOR, true);
        
        ofSetColor(ofColor::fromHsb(hue, saturation, 255.0f));
        ofNoFill();
        ofSetLineWidth(3.0f);
        
        ofLine(lastEndPoint, mousePoint);
        lastEndPoint = mousePoint;
        
        if (radius > 0.0f){
            ofPolyline randomShape;
            randomShape.addVertex(mousePoint);
            for (int s=0; s<4; s++){
                float angle = M_PI*2.0f*ofRandomf();
                randomShape.addVertex(ofPoint(mousePoint.x + cosf(angle)*radius, mousePoint.y + sinf(angle)*radius));
            }
            randomShape.close();
            randomShape.draw(); 
        }
    }
#else
    ofSetColor(255, 255, 255);
    drawHandSprites();
#endif
    
    if (showTrails){
        ofTexture & mainTex = mainFbo.getTextureReference(0);
        mainFbo.setActiveDrawBuffer(1);
        ofSetColor(255,255,255);
        blurShader.begin();
        blurShader.setUniformTexture("texSampler", mainTex, 1);
        mainTex.draw(0,0);
        blurShader.end();
    }
    
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    glDisable(GL_DEPTH_TEST);
    
    float lowFreq = audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_LOW)*2.0f;
    float bright = ofMap(lowFreq, 0.0f, 2.0f, 20.0f, 200.0f, true);
    ofBackgroundGradient(ofColor::fromHsb(180, 80, bright), ofColor::fromHsb(0, 0, 20));
    ofSetColor(255, 255, 255);
    mainFbo.draw(0, 0);
    
    
    if (debugMode){
        stringstream ss;
        ss << setprecision(2);
        ss << "Audio Signal Energy: " << audioAnalyzer.getSignalEnergy();
        ofDrawBitmapString(ss.str(), 20,30);
        ss.str(std::string());
        ss << "Audio Signal PSF: " << audioAnalyzer.getTotalPSF();
        ofDrawBitmapString(ss.str(), 20,45);
        ss.str(std::string());
        ss << "Region Energy -- Low: " << audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW) <<
        " Mid: " << audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_MID) << " High: " << audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH);
        ofDrawBitmapString(ss.str(), 20, 60);
        ss.str(std::string());
        ss << "Region PSF -- Low: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_LOW) <<
        " Mid: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_MID) << " High: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_HIGH);
        ofDrawBitmapString(ss.str(), 20, 75);
    }

}

void ofApplication::drawHandSprites()
{
    float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
    spriteColor = ofColor::fromHsb(hue, 255.0f, 255.0f);
    
    handPhysics->update();
    
    float highEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH)*150.0f;
    
    for (int i=0; i<handPhysics->getNumTrackedHands(); i++){
        
        ofSetColor(255,255,255);
        ofPoint handPos = handPhysics->getPhysicsStateForHand(i).handPositions[0];
        handPos *= ofGetWindowSize()/ofPoint(640,480);
        
        drawAudioBlobAtPoint(handPos);
        
        ofPoint hp = handPhysics->getNormalizedSpritePositionForHand(i);
        ofPoint hp1 = handPhysics->getNormalizedSpritePositionForHand(i, 1);
        hp *= ofGetWindowSize();
        hp1 *= ofGetWindowSize();
        
        ofSetColor(spriteColor);
        ofSetLineWidth(10.0f);
        
        if ((hp1 - hp).length() < 10.0f)
        {
            ofFill();
            ofCircle(hp, 5.0f);
        }
        else{
            ofLine(hp1, hp);
        }
        

    }
}

void ofApplication::drawAudioBlobAtPoint(ofPoint &point)
{
    ofSetColor(audioBlobColor);
    float highEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH);
    float radius = ofMap(highEnergy, 0.01f, 10.0f, 4.0f, MAX_BLOTCH_RADIUS, false);
    if (radius > 4.0f){
        ofSetLineWidth(2.0f);
        ofPolyline randomShape;
        randomShape.addVertex(point);
        for (int s=0; s<4; s++){
            float angle = M_PI*2.0f*ofRandomf();
            randomShape.addVertex(ofPoint(point.x + cosf(angle)*radius, point.y + sinf(angle)*radius));
        }
        randomShape.close();
        randomShape.draw();
    }
    else{
        ofFill();
        ofCircle(point, 4.0f);
    }
}

#pragma mark - Inputs

//--------------------------------------------------------------
void ofApplication::keyPressed(int key){
    switch (key) {
            
        case 'd':
            debugMode = !debugMode;
            
        case 't':
            showTrails = !showTrails;
            break;
            
        default:
            break;
    }
}

//--------------------------------------------------------------
void ofApplication::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApplication::mouseMoved(int x, int y){
#ifdef USE_MOUSE
    if (!ofGetMousePressed())
        lastEndPoint = ofPoint(x,y);
#endif
}

//--------------------------------------------------------------
void ofApplication::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApplication::mousePressed(int x, int y, int button){
#ifdef USE_MOUSE
    lastMousePoint = lastEndPoint;
    mouseVelocity = 0.0f;
#endif
}

//--------------------------------------------------------------
void ofApplication::mouseReleased(int x, int y, int button){
#ifdef USE_MOUSE
    mouseVelocity = 0.0f;
#endif
}

//--------------------------------------------------------------
void ofApplication::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApplication::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApplication::dragEvent(ofDragInfo dragInfo){ 

}