#include "ofApplication.h"
#include "ofxNDGraphicsUtils.h"

#define POI_MIN_SCALE_FACTOR 0.01

#define HANDS_MAX_SCALE_FACTOR 0.25

#define TRAIL_FBO_SCALE      1.25

static int inputDeviceId = 0;

void ofApplicationSetAudioInputDeviceId(int deviceId){
    inputDeviceId = deviceId;
}

//--------------------------------------------------------------

float midiToOnePoleTc(float midiValue, float minMs, float maxMs)
{
    float midiNorm = ofMap(midiValue, 0, 127, 0.0f, 1.0f);
    float maxPow = log10f(maxMs/minMs);
    return 1.0f - (1.0f/(ofGetFrameRate()*minMs*powf(10.0f,midiNorm*maxPow)*0.001f));
}   

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
    
#ifdef USE_KINECT
    // prevents crashing on exit (sometimes)
    kinectOpenNI.stop();
    kinectOpenNI.waitForThread();
#endif
}

void ofApplication::setup(){
    
    ofSetVerticalSync(true);
    ofEnableSmoothing();
    ofEnableArbTex();
    
    // setup animation parameters
    debugMode = false;
    showTrails = true;
    handsColor = ofColor(220,220,220);
    poiMaxScaleFactor = 0.1f;
    userShapeScaleFactor = 1.1f;
    
    ofSetCircleResolution(50);
    
    trailColorDecay = 0.975f;
    trailAlphaDecay = 0.98f;
    trailMinAlpha = 0.03f;
    
    trailVelocity = ofPoint(0.0f,80.0f);
    trailScale = ofPoint(-0.1f, -0.1f);
    trailScaleAnchor = ofPoint(0.5f, 0.5f);
    
#ifdef USE_MOUSE
    mouseVelocity = 0.0f;
#endif
    
    ofFbo::Settings fboSettings;
    fboSettings.width = ofGetWidth();
    fboSettings.height = ofGetHeight();
    fboSettings.useDepth = false;
    fboSettings.useStencil = false;
    fboSettings.depthStencilAsTexture = false;
    fboSettings.numColorbuffers = 1;
    fboSettings.internalformat = GL_RGBA;
        
    mainFbo.allocate(fboSettings);
    mainFbo.begin();
    mainFbo.activateAllDrawBuffers();
    ofClear(0,0,0,0);
    mainFbo.end();
    
    fboSettings.numColorbuffers = 2;
    fboSettings.width = ofGetWidth()*TRAIL_FBO_SCALE;
    fboSettings.height = ofGetHeight()*TRAIL_FBO_SCALE;
    fboSettings.internalformat = GL_RGBA32F_ARB;
    trailsFbo.allocate(fboSettings);
    trailsFbo.begin();
    trailsFbo.activateAllDrawBuffers();
    ofClear(0, 0, 0, 0);
    trailsFbo.end();
    
    fboSettings.numColorbuffers = 1;
    fboSettings.width = 640;
    fboSettings.height = 480;
    fboSettings.internalformat = GL_RGBA;
    userFbo.allocate(fboSettings);
    userFbo.begin();
    userFbo.activateAllDrawBuffers();
    ofClear(0,0,0,0);
    userFbo.end();
    
    trailsShader.load("shaders/vanilla.vert", "shaders/trails.frag");
    userMaskShader.load("shaders/vanilla.vert", "shaders/userDepthMask.frag");
    gaussianBlurShader.load("shaders/vanilla.vert", "shaders/gaussian.frag");
    
    // midi setup
    midiIn.openPort(3);
    midiIn.addListener(this);
    
    // audio setup
    audioSensitivity = 1.0f;
    
    ofxAudioAnalyzer::Settings audioSettings;
    audioSettings.stereo = true;
    audioSettings.inputDeviceId = inputDeviceId;
    audioSettings.bufferSize = 512;
    audioAnalyzer.setup(audioSettings);
    
    audioAnalyzer.setAttackInRegion(10, AA_FREQ_REGION_LOW);
    audioAnalyzer.setReleaseInRegion(250, AA_FREQ_REGION_LOW);
    audioAnalyzer.setAttackInRegion(5, AA_FREQ_REGION_MID);
    audioAnalyzer.setReleaseInRegion(120, AA_FREQ_REGION_MID);
    audioAnalyzer.setAttackInRegion(1, AA_FREQ_REGION_HIGH);
    audioAnalyzer.setReleaseInRegion(80, AA_FREQ_REGION_HIGH);
    
    // kinect setup
#ifdef USE_KINECT    
    
    kinectDriver.setup();
    kinectAngle = 0;
    
    kinectOpenNI.setup();
    kinectOpenNI.addImageGenerator();
    kinectOpenNI.addDepthGenerator();
    kinectOpenNI.setDepthColoring(COLORING_GREY);
    
    kinectOpenNI.setThreadSleep(10000);
    kinectOpenNI.setSafeThreading(false);
    kinectOpenNI.setRegister(true);
    kinectOpenNI.setMirror(true);
    
#ifdef USE_USER_TRACKING
    // setup user generator
    kinectOpenNI.addUserGenerator();
    kinectOpenNI.setMaxNumUsers(1);
    kinectOpenNI.setUseMaskPixelsAllUsers(true);
    kinectOpenNI.setUseMaskTextureAllUsers(true);
    kinectOpenNI.setUsePointCloudsAllUsers(false);
    kinectOpenNI.setSkeletonProfile(XN_SKEL_PROFILE_UPPER);
    kinectOpenNI.setUserSmoothing(0.2);
#else
    // hands generator
    kinectOpenNI.addHandsGenerator();
    kinectOpenNI.addAllHandFocusGestures();
    kinectOpenNI.setMaxNumHands(2);
    kinectOpenNI.setMinTimeBetweenHands(50);
#endif

    
    kinectOpenNI.start();
    
#ifdef USE_USER_TRACKING
    handPhysics = new ofxHandPhysicsManager(kinectOpenNI, true);
#else
    handPhysics = new ofxHandPhysicsManager(kinectOpenNI, false);
#endif
    
    handPhysics->restDistance = 0.0f;
    handPhysics->springCoef = 100.0f;
    handPhysics->smoothCoef = 0.5f;
    handPhysics->friction = 0.08f;
    handPhysics->gravity = ofVec2f(0,7000.0f);
    handPhysics->physicsEnabled = true;
#endif
}

//--------------------------------------------------------------
void ofApplication::update(){

    elapsedPhase = 2.0*M_PI*ofGetElapsedTimef();

#ifdef USE_KINECT
    kinectOpenNI.update();
    handPhysics->update();    
    blurUserOutline();
 #endif
    
    // draw to FBOs
    glDisable(GL_DEPTH_TEST);
    ofDisableBlendMode();
    
    if (showTrails){
        beginTrails();
        drawPoiSprites();
        endTrails();
    }
    
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    
    mainFbo.begin();
    ofClear(0,0,0,0);
    
    ofSetColor(50,50,50,200);
    
    float lowF = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW)*audioSensitivity;
    float scale = debugMode ? 1.0 : ofMap(lowF, 0.5f, 2.0f, 1.0f, userShapeScaleFactor);
    
    ofPushMatrix();
    ofScale(scale, scale);
    ofTranslate(-ofPoint(mainFbo.getWidth(), mainFbo.getHeight())*(scale - 1.0f)/2.0f);
    drawUserOutline();
    ofPopMatrix();
    
    if (showTrails){
        ofSetColor(255, 255, 255);
        drawTrails();
    }

    //drawHandSprites();
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    glDisable(GL_DEPTH_TEST);
    ofSetColor(255, 255, 255);
    ofEnableAlphaBlending();
    
    float lowF = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW)*audioSensitivity;
    float bright = ofMap(lowF, 0.1f, 2.0f, 60.0f, 160.0f, true);
    ofBackgroundGradient(ofColor::fromHsb(180, 80, bright), ofColor::fromHsb(0, 0, 20));
    
    // Draw the main FBO
    mainFbo.draw(0, 0);

    if (debugMode){
        
        kinectOpenNI.drawSkeletons(0, 0, ofGetWidth(), ofGetHeight());
        
        ofSetColor(255, 255, 255);
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
        
        ofDrawBitmapString("Frame Rate: " + ofToString(ofGetFrameRate()), 20, 100);
        
    }

}

void ofApplication::beginTrails()
{
    ofDisableBlendMode();
    ofSetColor(255,255,255);
    ofFill();
    
    trailsFbo.begin();
    trailsFbo.setActiveDrawBuffer(0);
    ofClear(0,0,0,0);

    ofTexture & fadingTex = trailsFbo.getTextureReference(1);
    
    ofPoint trailOffset = trailVelocity*ofGetLastFrameTime();
    
    ofPushMatrix();        

    ofTranslate(trailOffset);
    
    ofPoint scaleOffset = ofPoint(1.0f,1.0f) + (trailScale*ofGetLastFrameTime());
    
    ofScale(scaleOffset.x, scaleOffset.y);
    ofTranslate(-(trailScaleAnchor*ofPoint(trailsFbo.getWidth(),trailsFbo.getHeight())*(scaleOffset - ofPoint(1.0,1.0))));
    
    trailsShader.begin();
    trailsShader.setUniformTexture("texSampler", fadingTex, 1);
    trailsShader.setUniform1f("alphaDecay", trailAlphaDecay);
    trailsShader.setUniform1f("colorDecay", trailColorDecay);
    trailsShader.setUniform1f("alphaMin", trailMinAlpha);
    int w = trailsFbo.getWidth();
    int h = trailsFbo.getHeight();
    drawBillboardRect(0, 0, w, h, w, h);
    trailsShader.end();
    
    ofPopMatrix();
    
    // for other drawing methods to be scaled properly
    ofPushMatrix();
    ofPoint trailTrans = ofPoint(ofGetWidth(), ofGetHeight())*(TRAIL_FBO_SCALE - 1.0f)/2.0f;
    ofTranslate(trailTrans);
}

void ofApplication::endTrails()
{
    ofPopMatrix();

    ofDisableBlendMode();
    ofSetColor(255,255,255);
    trailsFbo.setActiveDrawBuffer(1);
    trailsFbo.getTextureReference(0).draw(0,0);
    trailsFbo.end();
}

void ofApplication::drawTrails()
{
    ofPushMatrix();
    ofPoint trailTrans = -ofPoint(ofGetWidth(), ofGetHeight())*(TRAIL_FBO_SCALE - 1.0f)/2.0f;
    ofTranslate(trailTrans);
    ofTexture & trailTex = trailsFbo.getTextureReference(0);
    trailTex.draw(0,0);
    ofPopMatrix();
}


void ofApplication::blurUserOutline()
{
    if (kinectOpenNI.getNumTrackedUsers() == 0)
        return;
    
    if (kinectOpenNI.getTrackedUser(0).isCalibrating())
        return;
    
    ofDisableBlendMode();
    
    ofTexture & depthTex = kinectOpenNI.getDepthTextureReference();
    ofTexture & maskTex = kinectOpenNI.getTrackedUser(0).getMaskTextureReference();
    float lowF = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW);
    
    userFbo.begin();
    
    // ===== mask =====
    userMaskShader.begin();
    userMaskShader.setUniformTexture("depthTexture", depthTex, 1);
    userMaskShader.setUniformTexture("maskTexture", maskTex, 2);
    drawBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    userMaskShader.end();
    
    // ===== blur =====
    gaussianBlurShader.begin();
    
    float blurAmt = ofMap(lowF, 0.1f, 3.0f, 0.01f, 15.0f, true);
    
    gaussianBlurShader.setUniform1f("sigma", blurAmt);
    gaussianBlurShader.setUniform1f("nBlurPixels", 15.0f);
    gaussianBlurShader.setUniform1i("isVertical", 0);
    gaussianBlurShader.setUniformTexture("blurTexture",  userFbo.getTextureReference(), 1);
    
    drawBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    gaussianBlurShader.setUniform1i("isVertical", 1);
    gaussianBlurShader.setUniformTexture("blurTexture", userFbo.getTextureReference(), 1);
    
    drawBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    gaussianBlurShader.end();
    
    userFbo.end();
}

void ofApplication::drawPoiSprites()
{
    float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
    poiSpriteColor = ofColor::fromHsb(hue, 255.0f, 255.0f);
    
    float highPSF = audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_HIGH)*audioSensitivity;
    float shapeRadius = ofMap(highPSF, 0.3f, 4.0f, POI_MIN_SCALE_FACTOR*ofGetWidth(), poiMaxScaleFactor*ofGetWidth(), true);
    
#ifdef USE_KINECT
    for (int i=0; i<handPhysics->getNumTrackedHands(); i++)
    {

        ofPoint hp = handPhysics->getNormalizedSpritePositionForHand(i);
        ofPoint hp1 = handPhysics->getNormalizedSpritePositionForHand(i, 1);
        hp *= ofGetWindowSize();
        hp1 *= ofGetWindowSize();
        
        ofxHandPhysicsManager::ofxHandPhysicsState physState = handPhysics->getPhysicsStateForHand(i);
        float spriteVel = physState.spriteVelocity.length();
        //float drawAlpha = ofMap(spriteVel, 0, 500, 180, 255);
        //ofSetColor(poiSpriteColor.r, poiSpriteColor.g, poiSpriteColor.b, drawAlpha);

#else
    {
        ofPoint hp = (ofGetWindowSize()/2.0f) + ofPoint(cosf(elapsedPhase/2.0f), sinf(elapsedPhase/2.0f))*100.0f;
        ofPoint hp1 = hp;
#endif
        
        ofSetColor(poiSpriteColor);
        ofFill();
        
        
        // --------- fake "waveform" drawing algorithm -------------
        ofVec2f pDiff = hp - hp1;

        float pointDistance = pDiff.length();
        int nSegments = 5;
        
        ofPolyline spriteShape;
        spriteShape.addVertex(ofPoint(0,0));
        
        if (pointDistance > 5.0f){
            nSegments = MAX(5, pointDistance/15);
            for (int n=0; n<nSegments; n++){
                spriteShape.lineTo(ofPoint(pointDistance*(n+1)/(nSegments+1), ofRandom(-shapeRadius, shapeRadius)));
            }
            spriteShape.lineTo(ofPoint(pointDistance, 0));
        }
        else{
            for (int n=0; n<nSegments; n++){
                float angle = ofRandom(0, M_PI*2.0);
                spriteShape.lineTo(ofPoint(cosf(angle),sinf(angle))*shapeRadius);
            }
            spriteShape.close();
        }
        
        float dirAngle = ofVec2f(1.0f,0.0f).angle(pDiff);
        
        ofSetLineWidth(4.0f);
        ofPushMatrix();
        ofTranslate(hp1);
        ofRotate(dirAngle, 0, 0, 1);
        spriteShape.draw();
        ofPopMatrix();
        
    }
}

void ofApplication::drawHandSprites()
{
    ofSetColor(handsColor);
    float midEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_MID)*audioSensitivity;
    float radius = ofMap(midEnergy, 0.1f, 5.0f, 4.0f, HANDS_MAX_SCALE_FACTOR*ofGetWidth(), false);
    
#ifdef USE_KINECT
    for (int i=0; i<handPhysics->getNumTrackedHands(); i++)
    {
        
        ofPoint handPos = handPhysics->getPhysicsStateForHand(i).handPositions[0];
        handPos *= ofGetWindowSize()/ofPoint(640,480);
#else
    {
        ofPoint handPos = ofGetWindowSize()/2.0f;
#endif
        if (radius > 4.0f){
            ofSetLineWidth(3.0f);
            ofPolyline randomShape;
            randomShape.addVertex(ofPoint(0,0));
            for (int s=0; s<4; s++){
                float angle = M_PI*2.0f*ofRandomf();
                randomShape.addVertex(ofPoint(cosf(angle)*radius, sinf(angle)*radius));
            }
            randomShape.close();
            ofPushMatrix();
            ofTranslate(handPos);
            randomShape.draw();
            ofPopMatrix();
        }
        else{
            ofFill();
            ofCircle(handPos, 4.0f);
        }
    }
}
    
void ofApplication::drawUserOutline()
{
    userFbo.getTextureReference().draw(0,0,mainFbo.getWidth(),mainFbo.getHeight());
}

#pragma mark - Inputs

//--------------------------------------------------------------
void ofApplication::keyPressed(int key){
    
    switch (key) {
            
        case OF_KEY_UP:
            kinectAngle = CLAMP(kinectAngle + 1, -30, 30);
            kinectDriver.setTiltAngle(kinectAngle);
            break;
            
        case OF_KEY_DOWN:
            kinectAngle = CLAMP(kinectAngle - 1, -30, 30);
            kinectDriver.setTiltAngle(kinectAngle);
            break;
            
        case '=':
            audioSensitivity = CLAMP(audioSensitivity*1.1f, 0.5f, 4.0f);
            break;
            
        case '-':
            audioSensitivity = CLAMP(audioSensitivity*0.9f, 0.5f, 4.0f);
            break;
            
        case 'd':
            debugMode = !debugMode;
            midiIn.setVerbose(debugMode);
            break;
            
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

void ofApplication::newMidiMessage(ofxMidiMessage& msg)
{
    // filter by control number (any channel)
    switch (msg.control) {
        case 1:
            break;
            
            
        // Trail parameters
        case 30:
            trailVelocity.x = ofMap((float)msg.value, 0, 127, -300.0f, 300.0f);
            break;
            
        case 31:
            trailVelocity.y = ofMap((float)msg.value, 0, 127, -300.0f, 300.0f);
            break;
            
        case 32:
            trailScaleAnchor.x = ofMap((float)msg.value, 0, 127, 0.0f, 1.0f);
            break;
            
        case 33:
            trailScaleAnchor.y = ofMap((float)msg.value, 0, 127, 0.0f, 1.0f);
            break;
            
        case 34:
            trailScale = ofPoint(1,1)*ofMap((float)msg.value, 0, 127, -0.5f, 0.5f);
            break;
            
        case 40:
            trailAlphaDecay = midiToOnePoleTc((float)msg.value, 10, 10000);
            break;
            
        case 41:
            trailColorDecay = midiToOnePoleTc((float)msg.value, 10, 10000);
            break;
            
        case 42:
            trailMinAlpha = ofMap((float)msg.value, 0, 127, 0.02f, 0.15f);
            break;
            
            
        // Audio tuning
        case 120:
            audioSensitivity = ofMap((float)msg.value, 0, 127, 0.5f, 4.0f, true);
            break;
            
        default:
            break;
    }
}
