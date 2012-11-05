#include "ofApplication.h"

#define MAX_BLOTCH_RADIUS    150.0f

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
    
    ofSetFrameRate(30);
    ofSetVerticalSync(true);
    ofEnableSmoothing();
    
    // setup animation parameters
    debugMode = false;
    showTrails = true;
    audioBlobColor = ofColor(180,180,180);
    
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
    
    blurShader.load("shaders/blur.vert", "shaders/blur.frag");
    blurDirection = ofPoint(0,0);
    blurVelocity = ofPoint(5.0f,5.0f);
    
    // audio setup
    ofxAudioAnalyzer::Settings audioSettings;
    audioSettings.stereo = true;
    audioSettings.inputDeviceId = inputDeviceId;
    audioSettings.bufferSize = 512;
    audioAnalyzer.setup(audioSettings);
    
    // kinect setup
#ifdef USE_KINECT
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
    handPhysics->smoothCoef = 0.75f;
    handPhysics->friction = 0.03f;
    handPhysics->gravity = ofVec2f(0,800.0f);
    handPhysics->physicsEnabled = true;
#endif
}

//--------------------------------------------------------------
void ofApplication::update(){
    
#ifdef USE_KINECT
    handPhysics->update();
#endif
    
    elapsedPhase = 2.0*M_PI*ofGetElapsedTimef();

    // draw to FBO
    
    glDisable(GL_DEPTH_TEST);
    
    mainFbo.begin();
    ofFill();
    mainFbo.setActiveDrawBuffer(0);
    ofDisableAlphaBlending();
    ofClear(0,0,0,0);
    ofSetColor(255,255,255);
    
    ofTexture & fadingTex = mainFbo.getTextureReference(1);
    
    if (showTrails){
        if (ofGetFrameNum() % 180 == 0)
        {
            blurVelocity = ofPoint(1.0f,1.0f)*ofRandom(5.0f, 80.0f);
            blurDirection = ofPoint(1.0f,1.0f)*ofRandom(-0.2f, 0.2f);
        }
        
        ofPoint scaledBlurVelocity = blurVelocity/10000.0f;
        ofPoint scaledBlurDirection = (ofPoint(0.5,0.5) + blurDirection) * scaledBlurVelocity;
        
        ofPushMatrix();
        ofScale(1.0f + scaledBlurVelocity.x, 1.0f + scaledBlurVelocity.y);
        
        // re center
        ofPoint translation = -(ofPoint(ofGetWidth(), ofGetHeight())*scaledBlurDirection);
        ofTranslate(translation);
        fadingTex.draw(0,0);
        ofPopMatrix();
    }

    ofSetColor(255, 255, 255);
    drawHandSprites();
    
    if (showTrails){
        ofTexture & mainTex = mainFbo.getTextureReference(0);
        mainFbo.setActiveDrawBuffer(1);
//        mainTex.draw(0,0);
//        //ofClear(0,0,0,0);
//        glEnable(GL_BLEND);
//        ofSetColor(0, 0, 0, 254);
//        glBlendFunc(GL_ZERO,GL_SRC_ALPHA);
//        ofRect(0, 0, mainFbo.getWidth(), mainFbo.getHeight());
//        glDisable(GL_BLEND);
//
        
        
        //ofEnableAlphaBlending();
        ofSetColor(255,255,255);
        blurShader.begin();
        blurShader.setUniformTexture("texSampler", mainTex, 1);
        drawBillboardRect(0, 0, mainFbo.getWidth(), mainFbo.getHeight());
        blurShader.end();
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
    
    float lowFreq = audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_LOW)*2.0f;
    float bright = ofMap(lowFreq, 0.0f, 2.5f, 20.0f, 200.0f, true);
    ofBackgroundGradient(ofColor::fromHsb(180, 80, bright), ofColor::fromHsb(0, 0, 20));
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
        
    float highEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH)*150.0f;
    
    for (int i=0; i<handPhysics->getNumTrackedHands(); i++){
        
        ofSetColor(255,255,255);
        
#ifdef USE_KINECT
        ofPoint hp = handPhysics->getNormalizedSpritePositionForHand(i);
        ofPoint hp1 = handPhysics->getNormalizedSpritePositionForHand(i, 1);
#else
        // TODO: Make these move
        ofPoint hp = ofPoint(0,0);
        ofPoint hp1 = ofPoint(0,1);
#endif
        hp *= ofGetWindowSize();
        hp1 *= ofGetWindowSize();
        ofSetColor(spriteColor);
        ofSetLineWidth(10.0f);
        
        if ((hp1 - hp).length() < 5.0f)
        {
            ofFill();
            ofCircle(hp, 5.0f);
        }
        else{
            ofLine(hp1, hp);
        }
    }
}

void ofApplication::drawAudioBlobs()
{
    ofSetColor(audioBlobColor);
    float highEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH);
    float radius = ofMap(highEnergy, 0.1f, 10.0f, 4.0f, MAX_BLOTCH_RADIUS, false);
    
    for (int i=0; i<handPhysics->getNumTrackedHands(); i++){
        
#ifdef USE_KINECT
        ofPoint handPos = handPhysics->getPhysicsStateForHand(i).handPositions[0];
        handPos *= ofGetWindowSize()/ofPoint(640,480);
#else
        ofPoint handPos = ofPoint(0,0);
#endif
        if (radius > 4.0f){
            ofSetLineWidth(5.0f);
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