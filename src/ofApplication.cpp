#include "ofApplication.h"
#include <stdlib.h>

#define POI_MIN_SCALE_FACTOR 0.01

#define HANDS_MAX_SCALE_FACTOR 0.4

#define TRAIL_FBO_SCALE      1.25

#define SKEL_NUM_CIRCLES_PER_LIMB   4



static int s_inputAudioDeviceId = 0;
static int s_oscListenPort = 9010;

void ofApplicationSetAudioInputDeviceId(int deviceId){
    s_inputAudioDeviceId = deviceId;
}

void ofApplicationSetOSCListenPort(int listenPort){
    s_oscListenPort = listenPort;
}

//--------------------------------------------------------------

float toOnePoleTC(float value, float minMs, float maxMs)
{
    float midiNorm = ofMap(value, 0.0f, 1.0f, 0.0f, 1.0f, true);
    float maxPow = log10f(maxMs/minMs);
    return 1.0f - (1.0f/(ofGetFrameRate()*minMs*powf(10.0f,midiNorm*maxPow)*0.001f));
}   

ofApplication::~ofApplication()
{
#ifdef USE_KINECT
    // prevents crashing on exit (sometimes)
    kinectOpenNI.stop();
    kinectOpenNI.waitForThread();
#endif
}

void ofApplication::setup(){
    
    // Renderer
    ofSetVerticalSync(true);
    ofSetFrameRate(60);
    ofEnableSmoothing();
    ofEnableArbTex();
    ofSetCircleResolution(32);
    
    // resources
    paperImage.loadImage("white-paper.jpg");
    
    // setup animation parameters
    debugMode = false;
    
    // FLAGS
    bDrawUserOutline = false;
    bTrailUserOutline = false;

    // TRAILS
    trailColorDecay = 0.8f;
    trailAlphaDecay = 0.92f;
    trailMinAlpha = 0.03f;
    trailVelocity = ofPoint(0.0f,0.0f);
    trailZoom = 0.0f;

    // USER OUTLINE
    userOutlineColorHSB = ofxNDHSBColor(0,0,255);
    userShapeScaleFactor = 1.1f;
    strobeLastDrawTime = 0;
    strobeIntervalMs = 70;
    
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
    
    // osc setup
    oscIn.setup(s_oscListenPort);
    
    // audio setup
    audioSensitivity = 1.0f;
    
    ofxAudioAnalyzer::Settings audioSettings;
    audioSettings.stereo = true;
    audioSettings.inputDeviceId = s_inputAudioDeviceId;
    audioSettings.bufferSize = 512;
    audioAnalyzer.setup(audioSettings);
    
    audioAnalyzer.setAttackInRegion(10, AA_FREQ_REGION_LOW);
    audioAnalyzer.setReleaseInRegion(150, AA_FREQ_REGION_LOW);
    audioAnalyzer.setAttackInRegion(5, AA_FREQ_REGION_MID);
    audioAnalyzer.setReleaseInRegion(80, AA_FREQ_REGION_MID);
    audioAnalyzer.setAttackInRegion(1, AA_FREQ_REGION_HIGH);
    audioAnalyzer.setReleaseInRegion(40, AA_FREQ_REGION_HIGH);
    
    // kinect setup
#ifdef USE_KINECT    
    
    //kinectDriver.setup();
    kinectAngle = 0;
    
    kinectOpenNI.setup();
    kinectOpenNI.addImageGenerator();
    kinectOpenNI.addDepthGenerator();
    kinectOpenNI.setDepthColoring(COLORING_GREY);
    
    kinectOpenNI.setThreadSleep(15000);
    kinectOpenNI.setSafeThreading(false);
    kinectOpenNI.setRegister(true);
    kinectOpenNI.setMirror(true);
    
#ifdef USE_USER_TRACKING
    // setup user generator
    kinectOpenNI.addUserGenerator();
    kinectOpenNI.setMaxNumUsers(2);
    kinectOpenNI.setUseMaskPixelsAllUsers(true);
    kinectOpenNI.setUseMaskTextureAllUsers(true);
    kinectOpenNI.setUsePointCloudsAllUsers(false);
    kinectOpenNI.setSkeletonProfile(XN_SKEL_PROFILE_ALL);
    kinectOpenNI.setUserSmoothing(0.5);
#else
    // hands generator
    kinectOpenNI.addHandsGenerator();
    kinectOpenNI.addAllHandFocusGestures();
    kinectOpenNI.setMaxNumHands(2);
    kinectOpenNI.setMinTimeBetweenHands(50);
#endif
    
    kinectOpenNI.start();

#endif
    
}

//--------------------------------------------------------------
void ofApplication::update(){
    
    float elapsedTime = ofGetElapsedTimef();

    audioLowEnergy = ofMap(audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW)*audioSensitivity, 0.15f, 1.0f, 0.0f, 1.0f, true);
    audioMidEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_MID)*audioSensitivity;
    audioHiPSF = ofMap(audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_HIGH)*audioSensitivity, 0.3f, 4.0f, 0.0f, 1.0f, true);
    elapsedPhase = 2.0*M_PI*elapsedTime;
    
    processOscMessages();

#ifdef USE_KINECT
    screenNormScale = ofGetWindowSize()/ofPoint(kinectOpenNI.getWidth(), kinectOpenNI.getHeight());
    screenNormScale.z = 1.0f;
    kinectOpenNI.update();
    if(bDrawUserOutline) updateUserOutline();
 #endif
    
    // draw to FBOs
    glDisable(GL_DEPTH_TEST);
    
    beginTrails();
    drawShapeSkeletons();
    endTrails();
    
    mainFbo.begin();
    ofClear(0,0,0,0);
    drawTrails();
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    
    float elapsedTime = ofGetElapsedTimef();

    glDisable(GL_DEPTH_TEST);
    ofEnableAlphaBlending();
    ofFill();
    
    // draw paper texture
    ofSetColor(255, 255, 255);
    
    // don't draw if frame freeze is turned on
    bool movePaper = true;
    if (strobeIntervalMs > 1000.0f/60.0f){
        movePaper = (elapsedTime - strobeLastDrawTime >= strobeIntervalMs/1000.0f);
        if (movePaper){
            strobeLastDrawTime = elapsedTime;
            paperInset = ofPoint(ofRandom(0,paperImage.getWidth() - ofGetWidth()), ofRandom(0,paperImage.getHeight()-ofGetWidth()));
        }
    }
    paperImage.drawSubsection(0, 0,
                              ofGetWidth(), ofGetHeight(),
                              paperInset.x, paperInset.y,
                              ofGetWidth(), ofGetHeight());
        
    // Draw the main FBO
    mainFbo.draw(0, 0);

    if (debugMode){
        
#ifdef USE_KINECT
        kinectOpenNI.drawSkeletons(0, 0, ofGetWidth(), ofGetHeight());
#endif
        
        ofSetColor(0);
        stringstream ss;
        ss << setprecision(2);
        ss << "Audio Signal Energy: " << audioAnalyzer.getSignalEnergy();
        ofDrawBitmapString(ss.str(), 20,30);
        ss.str(std::string());
        ss << "Audio Signal PSF: " << audioAnalyzer.getTotalPSF();
        ofDrawBitmapString(ss.str(), 20,45);
        ss.str(std::string());
        ss << "Region Energy -- Low: " << audioLowEnergy << " Mid: " << audioMidEnergy << " High: " << audioHiEnergy;
        ofDrawBitmapString(ss.str(), 20, 60);
        ss.str(std::string());
        ss << "Kick Energy: " << audioAnalyzer.getKickEnergy();
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
    
    float zoomInc = trailZoom * ofGetLastFrameTime();
    ofPoint scaleOffset = ofPoint(1.0f+zoomInc ,1.0f + zoomInc);
    ofScale(scaleOffset.x, scaleOffset.y);
    ofTranslate(-0.5f*ofPoint(trailsFbo.getWidth(),trailsFbo.getHeight())*(scaleOffset - ofPoint(1.0,1.0)));
    
    trailsShader.begin();
    trailsShader.setUniformTexture("texSampler", fadingTex, 1);
    trailsShader.setUniform1f("alphaDecay", trailAlphaDecay);
    trailsShader.setUniform1f("colorDecay", trailColorDecay);
    trailsShader.setUniform1f("alphaMin", trailMinAlpha);
    int w = trailsFbo.getWidth();
    int h = trailsFbo.getHeight();
    ofxNDBillboardRect(0, 0, w, h, w, h);
    trailsShader.end();
    
    ofPopMatrix();
    
    // for other drawing methods to be scaled properly
    ofPushMatrix();
    ofPoint trailTrans = ofPoint(ofGetWidth(), ofGetHeight())*(TRAIL_FBO_SCALE - 1.0f)/2.0f;
    ofTranslate(trailTrans);
}

void ofApplication::endTrails()
{
    ofDisableBlendMode();
    ofPopMatrix();
    ofSetColor(255,255,255);
    trailsFbo.setActiveDrawBuffer(1);
    ofClear(0,0,0,0);
    trailsFbo.getTextureReference(0).draw(0,0);
    trailsFbo.end();
}

void ofApplication::drawTrails()
{
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofSetColor(255, 255, 255);
    ofPushMatrix();
    ofPoint trailTrans = -ofPoint(ofGetWidth(), ofGetHeight())*(TRAIL_FBO_SCALE - 1.0f)/2.0f;
    ofTranslate(trailTrans);
    ofTexture & trailTex = trailsFbo.getTextureReference(0);
    trailTex.draw(0,0);
    ofPopMatrix();
}


void ofApplication::updateUserOutline()
{
#ifdef USE_KINECT
    if (kinectOpenNI.getNumTrackedUsers() == 0)
        return;
    
    if (kinectOpenNI.getTrackedUser(0).isCalibrating())
        return;
    
    ofDisableBlendMode();
    
    ofTexture & depthTex = kinectOpenNI.getDepthTextureReference();
    ofTexture & maskTex = kinectOpenNI.getTrackedUser(0).getMaskTextureReference();
    
    userFbo.begin();
    ofClear(0,0,0,0);
    
    // ===== mask =====
    userMaskShader.begin();
    userMaskShader.setUniformTexture("depthTexture", depthTex, 1);
    userMaskShader.setUniformTexture("maskTexture", maskTex, 2);
    ofxNDBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    userMaskShader.end();
    
    // ===== blur =====
    gaussianBlurShader.begin();
    
    float blurAmt = 4.0f; //ofMap(audioLowEnergy, 0.0f, 1.0f, 0.01f, 15.0f, true);
    
    gaussianBlurShader.setUniform1f("sigma", blurAmt);
    gaussianBlurShader.setUniform1f("nBlurPixels", 15.0f);
    gaussianBlurShader.setUniform1i("isVertical", 0);
    gaussianBlurShader.setUniformTexture("blurTexture",  userFbo.getTextureReference(), 1);
    
    ofxNDBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    gaussianBlurShader.setUniform1i("isVertical", 1);
    gaussianBlurShader.setUniformTexture("blurTexture", userFbo.getTextureReference(), 1);
    
    ofxNDBillboardRect(0, 0, userFbo.getWidth(), userFbo.getHeight(), depthTex.getWidth(), depthTex.getHeight());
    
    gaussianBlurShader.end();
    
    userFbo.end();
#endif
}
  
void ofApplication::drawUserOutline()
{
#ifdef USE_KINECT
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    float scale = debugMode ? 1.0 : ofMap(audioLowEnergy, 0.0f, 1.0f, 1.0f, userShapeScaleFactor, true);
    ofSetColor(userOutlineColorHSB.getOfColor());
    ofPushMatrix();
    ofScale(scale, scale);
    ofTranslate(-ofPoint(mainFbo.getWidth(), mainFbo.getHeight())*(scale - 1.0f)/2.0f);
    userFbo.getTextureReference().draw(0,0,mainFbo.getWidth(),mainFbo.getHeight());
    ofPopMatrix();
#endif
}

void ofApplication::drawShapeSkeletons()
{
    ofEnableBlendMode(OF_BLENDMODE_ALPHA);
    ofNoFill();
    ofSetLineWidth(3.0f);
    
    float height = ofGetHeight();
    float depthScale = 1.0f;
    
    ofPoint currentCenter;
    ofVec2f currentOffset;
    ofColor currentColor = ofColor(0,0,0);
    float currentAngle = 0.0f;
    float currentRadius = 100.0f;
    float kickEnergy = audioLowEnergy*audioHiPSF;
    
    for (int u=0; u<kinectOpenNI.getNumTrackedUsers(); u++){
        
        ofxOpenNIUser & user = kinectOpenNI.getTrackedUser(u);
        if (user.isSkeleton()){
            
            // draw head as several circles
            // low freq jitters size and center point
            
            // get length of neck joint to base head size
            ofxOpenNILimb & neck = user.getLimb(LIMB_NECK);
            float neckLength = (neck.getStartJoint().getProjectivePosition()*screenNormScale - neck.getEndJoint().getProjectivePosition()*screenNormScale).length();
            
            currentCenter = user.getJoint(JOINT_HEAD).getProjectivePosition()*screenNormScale;
            
            // base depth scale off of head Z
            currentRadius = 0.45*neckLength;
            currentCenter.z = 0.0f;
            ofPushMatrix();
            ofTranslate(currentCenter);
            for (int c=0; c<SKEL_NUM_CIRCLES_PER_LIMB; c++)
            {
                currentAngle = ofRandom(0, 2*M_PI);
                currentOffset = c == -2 ? ofVec2f() : ofVec2f(cosf(currentAngle), sinf(currentAngle)).normalized()*(audioLowEnergy*0.15f + 0.04f)*2*currentRadius;
                currentColor.a = c == -2 ? 200 : 175;
                ofSetColor(currentColor);
                ofEllipse(currentOffset, currentRadius, currentRadius*2.0f);
            }
            ofPopMatrix();

            drawCirclesForLimb(user.getLimb(LIMB_LEFT_SHOULDER));
            drawCirclesForLimb(user.getLimb(LIMB_LEFT_UPPER_ARM));
            drawCirclesForLimb(user.getLimb(LIMB_LEFT_LOWER_ARM));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_SHOULDER));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_UPPER_ARM));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_LOWER_ARM));
            drawCirclesForLimb(user.getLimb(LIMB_LEFT_UPPER_TORSO));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_UPPER_TORSO));
            drawCirclesForLimb(user.getLimb(LIMB_LEFT_LOWER_TORSO));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_LOWER_TORSO));
            drawCirclesForLimb(user.getLimb(LIMB_PELVIS));
            drawCirclesForLimb(user.getLimb(LIMB_LEFT_UPPER_LEG));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_UPPER_LEG));
            drawCirclesForLimb(user.getLimb(LIMB_LEFT_LOWER_LEG));
            drawCirclesForLimb(user.getLimb(LIMB_RIGHT_LOWER_LEG));            
        }
    }
}

void ofApplication::drawCirclesForLimb(ofxOpenNILimb & limb)
{
    ofPoint limbStart = limb.getStartJoint().getProjectivePosition()*screenNormScale;
    ofPoint limbEnd = limb.getEndJoint().getProjectivePosition()*screenNormScale;
    limbStart.z = 0.0f;
    limbEnd.z = 0.0f;
    
    ofPoint center = limbStart.middle(limbEnd);
    float length = limbStart.distance(limbEnd)*2.2f;
    
    ofVec3f diff = limbStart - limbEnd;
    float angle = ofVec3f(0,-1,0).angle(diff);
    if (diff.x < 0){
        angle = 360 - angle;
    }
    
    ofPushMatrix();
    ofTranslate(center);
    ofRotateZ(angle);
    
    ofColor currentColor = ofColor(0,0,0);

    for (int c=0; c<SKEL_NUM_CIRCLES_PER_LIMB; c++){
        float currentAngle = ofRandom(0, 2*M_PI);
        ofPoint currentOffset = c == -2 ? ofVec2f() : ofVec2f(cosf(currentAngle), sinf(currentAngle)).normalized()*(audioLowEnergy*0.15f + 0.04f)*length;
        currentColor.a = c == -2 ? 200 : 175;
        ofSetColor(currentColor);
        ofEllipse(currentOffset, length*0.1, length);
    }
    ofPopMatrix();
    
}


#pragma mark - Inputs
    
void ofApplication::processOscMessages()
{
    while (oscIn.hasWaitingMessages())
    {
        ofxOscMessage m;
        oscIn.getNextMessage(&m);
        
        string a = m.getAddress();
        
        // ------ FLAGS/SWITCHES -------
        if (a == "/oF/drawUser/")
        {
            bDrawUserOutline = m.getArgAsFloat(0) != 0.0f;
        }
        else if (a == "/of/drawUserTrails")
        {
            bTrailUserOutline = m.getArgAsFloat(0) != 0.0f;
        }
        
        // ------- USER OUTLINE -------
        else if (a == "/oF/drawUser")
        {
            bDrawUserOutline = m.getArgAsFloat(0) != 0.0f;
        }
        else if (a == "/oF/drawUserTrails")
        {
            bTrailUserOutline = m.getArgAsFloat(0) != 0.0f;
        }
        
        // ------- EFFECTS ------
        
        else if (a == "/oF/strobeRate")
        {
            strobeIntervalMs = ofMap(m.getArgAsFloat(0), 0.0f, 1.0f, 10.0f, 250.0f, true);
        }
        else if (a == "/oF/trailVelocity")
        {
            // swap X and Y from touchOSC in landscape
            trailVelocity.set(ofVec3f(m.getArgAsFloat(1),m.getArgAsFloat(0))*300.0f);
        }
        else if (a == "/oF/trailZoom")
        {
            float oscv = m.getArgAsFloat(0);
            trailZoom = powf(fabs(oscv), 3.0f) * (oscv >= 0.0f ? 1.0f : -1.0f) * 10.0f;
        }
        else if (a == "/oF/trailAlphaFade")
        {
            trailAlphaDecay = toOnePoleTC(m.getArgAsFloat(0), 10, 10000);
        }
        else if (a == "/oF/trailColorFade")
        {
            trailColorDecay = toOnePoleTC(m.getArgAsFloat(0), 10, 10000);
        }
        else if (a == "/oF/trailMinAlpha")
        {
            trailMinAlpha = ofMap((float)m.getArgAsFloat(0), 0.0f, 1.0f, 0.02f, 0.15f);
        }
        
        // ------- AUDIO SENSITIVITY ------
        else if (a == "/oF/audioSensitivity")
        {
            audioSensitivity = ofMap(m.getArgAsFloat(0), 0.0f, 1.0f, 0.5f, 2.0f, true);
        }
    }
}
    
//--------------------------------------------------------------
void ofApplication::keyPressed(int key){
    
    switch (key) {
            
#ifdef USE_KINECT
        case OF_KEY_UP:
            kinectAngle = CLAMP(kinectAngle + 1, -30, 30);
            kinectDriver.setTiltAngle(kinectAngle);
            break;
            
        case OF_KEY_DOWN:
            kinectAngle = CLAMP(kinectAngle - 1, -30, 30);
            kinectDriver.setTiltAngle(kinectAngle);
            break;
#endif
            
        case '=':
            audioSensitivity = CLAMP(audioSensitivity*1.1f, 0.5f, 4.0f);
            break;
            
        case '-':
            audioSensitivity = CLAMP(audioSensitivity*0.9f, 0.5f, 4.0f);
            break;
            
        case 'd':
            debugMode = !debugMode;
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

