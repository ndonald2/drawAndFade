#include "ofApplication.h"
#include "ofxNDGraphicsUtils.h"

#define MAX_BLOTCH_RADIUS    0.1

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
    audioBlobColor = ofColor(220,220,220);
    
    ofSetCircleResolution(50);
    depthThresh = 0.2f;
    trailColorDecay = 0.99;
    trailAlphaDecay = 0.99;
    trailMinAlpha = 0.03;
        
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
    grayscaleThreshShader.load("shaders/vanilla.vert", "shaders/grayscaleThresh.frag");
    gaussianBlurShader.load("shaders/vanilla.vert", "shaders/gaussian.frag");
    
    trailVelocity = ofPoint(0.0f,25.0f);
    trailScale = ofPoint(1.0f, 1.0f);
    trailScaleAnchor = ofPoint(0.5f, 0.5f);
    
    // audio setup
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
    
#ifdef USE_USER_TRACKING
    // setup user generator
    kinectOpenNI.addUserGenerator();
    ofxOpenNIUser user;
    user.setUsePointCloud(true);
    user.setUseSkeleton(true);
    user.setUseMaskPixels(false);
    user.setUseMaskTexture(false);
    kinectOpenNI.setBaseUserClass(user);
#else
    // hands generator
    kinectOpenNI.addHandsGenerator();
    kinectOpenNI.addAllHandFocusGestures();
    kinectOpenNI.setMaxNumHands(2);
    kinectOpenNI.setMinTimeBetweenHands(50);
#endif

    kinectOpenNI.setThreadSleep(30000);
    kinectOpenNI.setSafeThreading(true);
    kinectOpenNI.setRegister(true);
    kinectOpenNI.setMirror(true);
    
    kinectOpenNI.start();
    
#ifdef USE_USER_TRACKING
    handPhysics = new ofxHandPhysicsManager(kinectOpenNI, true);
#else
    handPhysics = new ofxHandPhysicsManager(kinectOpenNI, false);
#endif
    
    handPhysics->restDistance = 40.0f;
    handPhysics->smoothCoef = 0.75f;
    handPhysics->friction = 0.03f;
    handPhysics->gravity = ofVec2f(0,800.0f);
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
    
    mainFbo.begin();
    ofClear(0,0,0,0);
    ofSetColor(200, 40, 20, 220);
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    drawUserOutline();
    if (showTrails){
        drawTrails();
    }
    drawHandSprites();
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    glDisable(GL_DEPTH_TEST);
    ofSetColor(255, 255, 255);
    ofEnableAlphaBlending();
    
    float lowF = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW);
    float bright = ofMap(lowF, 0.01f, 2.0f, 60.0f, 160.0f, true);
    ofBackgroundGradient(ofColor::fromHsb(180, 80, bright), ofColor::fromHsb(0, 0, 20));
    
    mainFbo.draw(0, 0);

    if (debugMode){
        
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
    trailsFbo.begin();
    trailsFbo.setActiveDrawBuffer(0);
    ofSetColor(255,255,255);
    ofFill();
    
    ofTexture & fadingTex = trailsFbo.getTextureReference(1);
            
    ofPoint trailOffset = trailVelocity*ofGetLastFrameTime();
    
    ofPushMatrix();        

    ofTranslate(trailOffset);
    ofScale(trailScale.x, trailScale.y);
    ofTranslate(-(trailScaleAnchor*ofGetWindowSize()*(trailScale - ofPoint(1.0,1.0))));
    
    trailsShader.begin();
    trailsShader.setUniformTexture("texSampler", fadingTex, 1);
    trailsShader.setUniform1f("alphaDecay", trailAlphaDecay);
    trailsShader.setUniform1f("colorDecay", trailColorDecay);
    trailsShader.setUniform1f("alphaMin", trailMinAlpha);
    drawBillboardRect(0, 0, ofGetWidth(), ofGetHeight(), fadingTex.getWidth(), fadingTex.getHeight());
    trailsShader.end();
    
    ofPopMatrix();    
}

void ofApplication::endTrails()
{
    ofSetColor(255,255,255);
    trailsFbo.setActiveDrawBuffer(1);
    trailsFbo.getTextureReference(0).draw(0,0);
    trailsFbo.end();
}

void ofApplication::blurUserOutline()
{
    ofDisableBlendMode();
    ofTexture & depthTex = kinectOpenNI.getDepthTextureReference();
    
    userFbo.begin();
    
    // ===== threshold =====
    grayscaleThreshShader.begin();
    grayscaleThreshShader.setUniform1f("threshold", depthThresh);
    grayscaleThreshShader.setUniformTexture("texture", depthTex, 1);
    
    drawBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    grayscaleThreshShader.end();
    
    // ===== blur =====
    gaussianBlurShader.begin();
    
    float blurAmt = ((sinf(elapsedPhase*0.5)*2.0f)-1.0f)*10.0f;
    
    gaussianBlurShader.setUniform1f("sigma", blurAmt);
    gaussianBlurShader.setUniform1f("nBlurPixels", 8.0f);
    gaussianBlurShader.setUniform1i("isVertical", 0);
    gaussianBlurShader.setUniformTexture("blurTexture",  userFbo.getTextureReference(), 1);
    
    drawBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    gaussianBlurShader.setUniform1i("isVertical", 1);
    gaussianBlurShader.setUniformTexture("blurTexture", userFbo.getTextureReference(), 1);
    
    drawBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    gaussianBlurShader.end();
    
    userFbo.end();
}

void ofApplication::drawTrails()
{
    ofSetColor(255, 255, 255);
    ofTexture & trailTex = trailsFbo.getTextureReference(0);
    trailTex.draw(0,0,mainFbo.getWidth(),mainFbo.getHeight());
}

void ofApplication::drawPoiSprites()
{
    float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
    spriteColor = ofColor::fromHsb(hue, 255.0f, 255.0f);
        
    float highPSF = audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_HIGH);

#ifdef USE_KINECT
    for (int i=0; i<handPhysics->getNumTrackedHands(); i++)
    {
    
        ofPoint hp = handPhysics->getNormalizedSpritePositionForHand(i);
        ofPoint hp1 = handPhysics->getNormalizedSpritePositionForHand(i, 1);
        hp *= ofGetWindowSize();
        hp1 *= ofGetWindowSize();
#else
    {
        ofPoint hp = (ofGetWindowSize()/2.0f) + ofPoint(cosf(elapsedPhase/2.0f), sinf(elapsedPhase/2.0f))*100.0f;
        ofPoint hp1 = hp;
#endif
        ofSetColor(spriteColor);
        ofFill();
        ofCircle(hp, ofMap(highPSF, 0.01f, 0.5f, 5.0f, 10.0f));
        
//        ofSetLineWidth(10.0f);
//        
//        if ((hp1 - hp).length() < 5.0f)
//        {
//            ofFill();
//            ofCircle(hp, 5.0f);
//        }
//        else{
//            ofLine(hp1, hp);
//        }
    }
}

void ofApplication::drawHandSprites()
{
    ofSetColor(audioBlobColor);
    float midEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_MID);
    float radius = ofMap(midEnergy, 0.1f, 5.0f, 4.0f, MAX_BLOTCH_RADIUS*ofGetWidth(), false);
    
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
            ofSetLineWidth(2.0f);
            ofPolyline randomShape;
            randomShape.addVertex(handPos);
            for (int s=0; s<4; s++){
                float angle = M_PI*2.0f*ofRandomf();
                randomShape.addVertex(ofPoint(handPos.x + cosf(angle)*radius, handPos.y + sinf(angle)*radius));
            }
            randomShape.close();
            randomShape.draw();
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
            depthThresh = MIN(depthThresh + 0.001, 1.0);
            break;
            
        case '-':
            depthThresh = MAX(depthThresh - 0.001, 0.0);
            break;
            
        case 'd':
            debugMode = !debugMode;
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