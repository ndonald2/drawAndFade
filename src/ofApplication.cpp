#include "ofApplication.h"
#include <stdlib.h>

#define TRAIL_FBO_SCALE                 1.25
#define SKEL_NUM_CIRCLES_PER_LIMB       4
#define SCANLINE_TIME                   8.0
#define PENCIL_MODE_TRAIL_DECAY         150
#define OLDCOMP_MODE_TRAIL_DECAY        120
#define SCANLINE_GRAD_H                 25
#define BOX_SCALE_MULT                  0.5

static int s_inputAudioDeviceId = 0;
static int s_oscListenPort = 9010;

void ofApplicationSetAudioInputDeviceId(int deviceId){
    s_inputAudioDeviceId = deviceId;
}

void ofApplicationSetOSCListenPort(int listenPort){
    s_oscListenPort = listenPort;
}

//--------------------------------------------------------------

float toOnePoleTC(float ms)
{
    return 1.0f - (1.0f/(ofGetFrameRate()*ms*0.001f));
}

// not actually linear, but exponential....
float toOnePoleTCLerp(float value, float minMs, float maxMs)
{
    float lerp = CLAMP(value,0.0f,1.0f);
    float maxPow = log10f(maxMs/minMs);
    return toOnePoleTC(minMs*powf(10.0f,lerp*maxPow));
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
    ofEnableSmoothing();
    ofEnableArbTex();
    ofSetCircleResolution(32);
    
    // resources
    paperImage.loadImage("white-paper.jpg");
    
    // setup animation parameters
    debugMode = false;
    skMode = SkeletonDrawModePencil;
    
    // FLAGS
    bDrawUserOutline = false;
    bTrailUserOutline = false;

    // TRAILS
    trailColorDecay = 1.0f;
    trailAlphaDecay = toOnePoleTC(PENCIL_MODE_TRAIL_DECAY);
    trailMinAlpha = 0.01f;
    trailVelocity = ofPoint(0.0f,0.0f);
    trailZoom = 0.0f;

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
    
    trailsShader.load("shaders/vanilla.vert", "shaders/trails.frag");
    
    // osc setup
    oscIn.setup(s_oscListenPort);
    
    // audio setup
    audioSensitivity = 1.0f;
    
    ofxAudioAnalyzer::Settings audioSettings;
    audioSettings.stereo = false;
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
    
    kinectDriver.setup();
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
    kinectOpenNI.setUserSmoothing(0.3);
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

    audioLowEnergy = ofMap(audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW)*audioSensitivity, 0.15f, 1.5f, 0.0f, 1.0f, true);
    audioMidEnergy = ofMap(audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_MID)*audioSensitivity, 0.1f, 1.25f, 0.0f, 1.0f, true);
    audioHiEnergy = ofMap(audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH)*audioSensitivity, 0.05f, 0.8f, 0.0f, 1.0f, true);
    audioHiPSF = ofMap(audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_HIGH)*audioSensitivity, 0.3f, 4.0f, 0.0f, 1.0f, true);
    elapsedPhase = 2.0*M_PI*elapsedTime;
    
    processOscMessages();

#ifdef USE_KINECT
    screenNormScale = ofGetWindowSize()/ofPoint(kinectOpenNI.getWidth(), kinectOpenNI.getHeight());
    screenNormScale.z = 1.0f;
    kinectOpenNI.update();
 #endif
    
    // draw to FBOs
    glDisable(GL_DEPTH_TEST);
    
    beginTrails();
    drawShapeSkeletons();
    endTrails();
    
    mainFbo.begin();
    ofClear(0,0,0,0);
    drawSceneBackground();
    drawTrails();
    // do this here so it goes on top of body
    if (skMode == SkeletonDrawModeOldComputer){
        drawScanLine();
    }
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    glDisable(GL_DEPTH_TEST);
    ofDisableBlendMode();
    ofFill();
    
    ofSetColor(255, 255, 255);
    
    // Draw the main FBO
    mainFbo.draw(0, 0);

    if (debugMode){
        
#ifdef USE_KINECT
        kinectOpenNI.drawSkeletons(0, 0, ofGetWidth(), ofGetHeight());
#endif
        
        float debugBrightness = 0;
        if (skMode == SkeletonDrawModeOldComputer){
            debugBrightness = 255;
        }
        
        ofSetColor(debugBrightness);
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
        ss << "LowPSF: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_LOW);
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

void ofApplication::drawSceneBackground()
{
    ofEnableAlphaBlending();

    if (skMode == SkeletonDrawModePencil)
    {
        ofSetColor(255, 255, 255);
        ofFill();
        
        // draw paper texture
        // don't draw if frame freeze is turned on
        float elapsedTime = ofGetElapsedTimef();
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
    }
}

void ofApplication::drawShapeSkeletons()
{
    ofEnableAlphaBlending();
    ofNoFill();
    ofSetLineWidth(3.0f);
    
    if (skMode == SkeletonDrawModePencil){
        glDisable(GL_DEPTH_TEST);
        ofSetColor(0,0,0,120);
    }
    else{
        glEnable(GL_DEPTH_TEST);
    }
    
#ifdef USE_KINECT
    for (int u=0; u<kinectOpenNI.getNumTrackedUsers(); u++){
        
        ofxOpenNIUser & user = kinectOpenNI.getTrackedUser(u);
        if (user.isSkeleton()){

            if (skMode == SkeletonDrawModePencil){
                
                audOffsetScale = powf(audioLowEnergy,1.75f);
                
                for (Limb i=LIMB_LEFT_UPPER_TORSO; i<LIMB_COUNT; i++){
                    drawShapeForLimb(user, i);
                }
            }
            else if (skMode == SkeletonDrawModeOldComputer)
            {
                
                audSizeScale = 1.0f + (powf(audioLowEnergy,1.75f)*BOX_SCALE_MULT);
                audColorScale = powf(audioHiEnergy,1.75f);
                
                // green outline
                ofColor lineColor = ofColor(60,65,60);
                lineColor.lerp(ofColor(0,255,0), CLAMP(audColorScale, 0, 1));
                ofSetColor(lineColor);

                drawShapeForLimb(user, LIMB_RIGHT_UPPER_ARM);
                drawShapeForLimb(user, LIMB_RIGHT_LOWER_ARM);
                drawShapeForLimb(user, LIMB_RIGHT_LOWER_LEG);
                drawShapeForLimb(user, LIMB_RIGHT_UPPER_LEG);
                drawShapeForLimb(user, LIMB_LEFT_LOWER_LEG);
                drawShapeForLimb(user, LIMB_LEFT_UPPER_LEG);
                drawShapeForLimb(user, LIMB_LEFT_UPPER_ARM);
                drawShapeForLimb(user, LIMB_LEFT_LOWER_ARM);
                drawShapeForLimb(user, LIMB_NECK);
                drawShapeForTorso(user);
            }
        }
    }
#else
    
    if (skMode == SkeletonDrawModePencil)
    {
       // something to debug here?
    }
    else if (skMode == SkeletonDrawModeOldComputer)
    {        
        audSizeScale = 1.0f + (powf(audioLowEnergy,1.75f)*0.2);
        audColorScale = powf(audioHiEnergy,1.75f);
        
        // green outline
        ofColor lineColor = ofColor(60,65,60);
        lineColor.lerp(ofColor(0,255,0), CLAMP(audColorScale, 0, 1));
        ofSetColor(lineColor);
        
        // centered rotating cube
        float spinDegrees = ((elapsedPhase*0.1 + audSizeScale)/(2.0*M_PI))*360;
        ofPushMatrix();
        ofTranslate(ofGetMouseX(), ofGetMouseY());
        ofScale(audSizeScale, audSizeScale, audSizeScale);
        ofRotateY(spinDegrees);
        ofBox(0, 0, 80);
        ofPopMatrix();
    }
    
#endif
}

void ofApplication::drawShapeForLimb(ofxOpenNIUser & user, Limb limbNumber)
{
    ofxOpenNILimb & limb = user.getLimb(limbNumber);
    
    ofPoint center;
    ofVec2f drawSize;
    float length = 0.0f;
    
    ofPoint limbStart = limb.getStartJoint().getProjectivePosition()*screenNormScale;
    ofPoint limbEnd = limb.getEndJoint().getProjectivePosition()*screenNormScale;
    
    float zFactor = limbEnd.z;
    
    limbStart.z = 0.0f;
    limbEnd.z = 0.0f;
    
    ofVec3f diff = limbStart - limbEnd;
    float angle = ofVec3f(0,-1,0).angle(diff);
    if (diff.x < 0){
        angle = 360 - angle;
    }
    
    if (limbNumber == LIMB_NECK){
        length = limbStart.distance(limbEnd)*0.5;
        center = user.getJoint(JOINT_HEAD).getProjectivePosition()*screenNormScale;
        drawSize.x = ofMap(zFactor,500,3500,0.11,0.01,true)*ofGetWidth();
        drawSize.y = drawSize.x*1.66f;
    }
    else{
        center = limbStart.middle(limbEnd);
        length = limbStart.distance(limbEnd)*2.2f;
        drawSize.x = ofMap(zFactor,500,3500,0.05,0.005,true)*ofGetWidth();
        drawSize.y = length;
    }
    
    center.z = 0;
    
    ofPushMatrix();

    if (skMode == SkeletonDrawModePencil){

        ofTranslate(center);
        ofRotateZ(angle);
        
        for (int c=0; c<SKEL_NUM_CIRCLES_PER_LIMB; c++){
            float currentAngle = ofRandom(0, 2*M_PI);
            ofPoint currentOffset = c == -2 ? ofVec2f() : ofVec2f(cosf(currentAngle), sinf(currentAngle)).normalized()*drawSize.y*CLAMP((audOffsetScale*0.2 + 0.04f),0,0.25f);
            ofEllipse(currentOffset, drawSize.x, drawSize.y);
        }
    }
    else if (skMode == SkeletonDrawModeOldComputer)
    {

        ofTranslate(center);
        ofRotateZ(angle);
        if (limbNumber == LIMB_NECK){
            float spinDegrees = ((elapsedPhase*0.1 + audSizeScale)/(2.0*M_PI))*360;
            ofRotateY(spinDegrees);
        }
        
        ofScale(drawSize.x, drawSize.y, drawSize.x);
        ofScale(audSizeScale, audSizeScale, audSizeScale);
        
        ofBox(0, 0, 0, 1);
    }
    
    ofPopMatrix();
}

void ofApplication::drawShapeForTorso(ofxOpenNIUser &user)
{
    ofPoint ul = user.getJoint(JOINT_LEFT_SHOULDER).getProjectivePosition()*screenNormScale;
    ofPoint ur = user.getJoint(JOINT_RIGHT_SHOULDER).getProjectivePosition()*screenNormScale;
    ofPoint ll = user.getJoint(JOINT_LEFT_HIP).getProjectivePosition()*screenNormScale;
    ofPoint lr = user.getJoint(JOINT_RIGHT_HIP).getProjectivePosition()*screenNormScale;
    ofPoint tc = user.getJoint(JOINT_NECK).getProjectivePosition()*screenNormScale;
    ofPoint cc = user.getJoint(JOINT_TORSO).getProjectivePosition()*screenNormScale;
    ul.z = ur.z = ll.z = lr.z = tc.z = cc.z = 0;
    
    // get angle
    ofVec3f bodyLine = tc - cc;
    float bodyAngle = ofVec3f(0,-1,0).angle(bodyLine);
    if (bodyLine.x < 0)
    {
        bodyAngle = 360 - bodyAngle;
    }

    // get size
    ofPoint bodySize = ofPoint(MIN(fabsf(ul.distance(ur)), fabsf(ll.distance(lr))), fabsf(ul.distance(ll)));
    
    ofPushMatrix();
    ofTranslate(cc.x, cc.y);
    ofRotateZ(bodyAngle);
    
    ofScale(bodySize.x, bodySize.y, bodySize.x);
    ofScale(audSizeScale, audSizeScale, audSizeScale);
    
    ofBox(0, 0, 0, 1);
    
    ofPopMatrix();
}

void ofApplication::drawScanLine()
{
    // draw the scanline
    ofEnableAlphaBlending();
    ofFill();
    ofSetColor(255,255,255);

    float lineY = ofGetElapsedTimef()/SCANLINE_TIME;
    lineY -= floorf(lineY);
    lineY *= ofGetHeight();
    
    float w = ofGetWidth();
    
	GLfloat verts[] = {
		0,lineY-SCANLINE_GRAD_H,
		w,lineY-SCANLINE_GRAD_H,
		w,lineY,
		0,lineY
	};
    
    ofFloatColor colors[] = {
        ofFloatColor(0,0,0,0),
        ofFloatColor(0,0,0,0),
        ofFloatColor(0, 1, 0, 0.75f),
        ofFloatColor(0, 1, 0, 0.75f)
    };
	
	glEnableClientState( GL_COLOR_ARRAY );
    glColorPointer(4, GL_FLOAT, sizeof(ofFloatColor), colors);
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, verts );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_COLOR_ARRAY );
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
            trailAlphaDecay = toOnePoleTCLerp(m.getArgAsFloat(0), 10, 10000);
        }
        else if (a == "/oF/trailColorFade")
        {
            trailColorDecay = toOnePoleTCLerp(m.getArgAsFloat(0), 10, 10000);
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
        
        case 'm':
            skMode = (SkeletonDrawMode)((skMode + 1) % SkeletonDrawModeNumModes);
            if (skMode == SkeletonDrawModePencil)
            {
                trailAlphaDecay = toOnePoleTC(PENCIL_MODE_TRAIL_DECAY);
            }
            else if (skMode == SkeletonDrawModeOldComputer)
            {
                trailAlphaDecay = toOnePoleTC(OLDCOMP_MODE_TRAIL_DECAY);
            }
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

