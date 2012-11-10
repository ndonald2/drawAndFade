#include "ofApplication.h"

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
    fboSettings.internalformat = GL_RGBA;
        
    mainFbo.allocate(fboSettings);
    mainFbo.begin();
    ofClear(0,0,0,0);
    mainFbo.setActiveDrawBuffer(1);
    ofClear(0,0,0,0);
    mainFbo.end();
    
    trailsShader.load("shaders/trails.vert", "shaders/trails.frag");
    userOutlineShader.load("shaders/userOutline.vert", "shaders/userOutline.frag");
    blurDirection = ofPoint(0,0);
    blurVelocity = ofPoint(5.0f,5.0f);
    
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
    kinectDriver.setTiltAngle(kinectAngle);
    
    kinectOpenNI.setup();
    kinectOpenNI.addImageGenerator();
    kinectOpenNI.addDepthGenerator();
    
    kinectOpenNI.addHandsGenerator();
    kinectOpenNI.addAllHandFocusGestures();
    kinectOpenNI.setMaxNumHands(2);
    
    kinectOpenNI.setUseDepthRawPixels(true);
    kinectOpenNI.setDepthColoring(COLORING_GREY);
    
    // setup user generator
//    kinectOpenNI.addUserGenerator();
//    kinectOpenNI.setMaxNumUsers(1);
//    
//    ofxOpenNIUser user;
//    user.setUsePointCloud(false);
//    user.setUseSkeleton(false);
//    user.setUseMaskPixels(true);
//    user.setUseMaskTexture(false);
//    kinectOpenNI.setBaseUserClass(user);
    
    kinectOpenNI.setThreadSleep(30000);
    kinectOpenNI.setSafeThreading(true);
    kinectOpenNI.setRegister(true);
    kinectOpenNI.setMirror(true);
    
    kinectOpenNI.start();
    
    handPhysics = new ofxHandPhysicsManager(kinectOpenNI);
    handPhysics->restDistance = 40.0f;
    handPhysics->smoothCoef = 0.75f;
    handPhysics->friction = 0.03f;
    handPhysics->gravity = ofVec2f(0,800.0f);
    handPhysics->physicsEnabled = true;
#endif
}

//--------------------------------------------------------------
void ofApplication::update(){
    
    double dTime = ofGetElapsedTimef() - ofGetLastFrameTime();
    
#ifdef USE_KINECT
    kinectOpenNI.update();
    handPhysics->update();
#endif
    
    elapsedPhase = 2.0*M_PI*ofGetElapsedTimef();

    // draw to FBO
    
    glDisable(GL_DEPTH_TEST);
    ofDisableAlphaBlending();
    
    mainFbo.begin();
    ofFill();
    mainFbo.setActiveDrawBuffer(0);
    ofClear(0,0,0,0);
    ofSetColor(255,255,255);
    
    ofTexture & fadingTex = mainFbo.getTextureReference(1);
    
    if (showTrails){
        if (ofGetFrameNum() % 180 == 0)
        {
            blurVelocity = ofPoint(1.0f,1.0f)*ofRandom(5.0f, 50.0f);
            blurDirection = ofPoint(1.0f,1.0f)*ofRandom(-0.2f, 0.2f);
        }
        
        
//        float lowEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW);
//        float velocityBump = ofMap(lowEnergy, 0.2f, 2.0f, 1.0f, 3.0f, true);
        
        ofPoint scaledBlurVelocity = blurVelocity*dTime*0.000001;
        ofPoint scaledBlurDirection = (ofPoint(0.5,0.5) + blurDirection) * scaledBlurVelocity;

        
        ofPushMatrix();
        ofScale(1.0f + scaledBlurVelocity.x, 1.0f + scaledBlurVelocity.y);
        
        // re center
        ofPoint translation = -(ofPoint(ofGetWidth(), ofGetHeight())*scaledBlurDirection);
        ofTranslate(translation);
        
        trailsShader.begin();
        trailsShader.setUniformTexture("texSampler", fadingTex, 1);
        drawBillboardRect(0, 0, ofGetWidth(), ofGetHeight());
        trailsShader.end();

        ofPopMatrix();
    }

    drawHandSprites();
    
    if (showTrails){
        ofSetColor(255,255,255);
        mainFbo.setActiveDrawBuffer(1);
        mainFbo.getTextureReference(0).draw(0,0);
    }

    mainFbo.setActiveDrawBuffer(0);
    drawAudioBlobs();
    
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    glDisable(GL_DEPTH_TEST);
    ofSetColor(255, 255, 255);
    ofEnableAlphaBlending();
    
    float lowF = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW);
    float bright = ofMap(lowF, 0.01f, 2.0f, 20.0f, 160.0f, true);
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

void ofApplication::drawHandSprites()
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
        ofSetLineWidth(2.0f);
        ofNoFill();
        ofLine(hp1, hp);
        ofPushMatrix();
        ofTranslate(hp);
        ofRotate(ofRandom(0, 360));
        ofEllipse(0, 0, 5.0f, ofMap(highPSF, 0.01f, 1.0f, 5.0f, 100.0f, true));
        ofPopMatrix();
        
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

void ofApplication::drawAudioBlobs()
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

void ofApplication::drawBillboardRect(int x, int y, int w, int h)
{
    GLfloat tex_coords[] = {
		0,0,
		w,0,
		w,h,
		0,h
	};
	GLfloat verts[] = {
		x,y,
		x+w,y,
		x+w,y+h,
		x,y+h
	};
	
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer(2, GL_FLOAT, 0, tex_coords );
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, verts );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
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