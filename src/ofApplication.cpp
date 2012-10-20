#include "ofApplication.h"

#define MAX_LINE_RADIUS    200.0f

static int audioInputIndex = 0;

void ofApplicationSetAudioInputIndex(int index){
    audioInputIndex = index;
}

//--------------------------------------------------------------
void ofApplication::setup(){
    
    ofSetFrameRate(60);
    ofSetVerticalSync(true);
    ofEnableAlphaBlending();
    ofEnableSmoothing();
    
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
    
    mouseVelocity = 0.0f;
    blurDirection = ofPoint(0,0);
    blurVelocity = 10;
    
    // audio setup
    audioInput.listDevices();
    audioInput.setDeviceID(audioInputIndex);
    int bufferSize = 256;
	left.assign(bufferSize, 0.0);
	right.assign(bufferSize, 0.0);
    audioInput.setup(this, 0, 2, 44100, bufferSize, 4);
    
    // kinect setup
    kinect.setRegistration(true);
    kinect.init(true, false);
    kinectAngle = 0;
    
	grayImage.allocate(kinect.width, kinect.height);
	grayThreshNear.allocate(kinect.width, kinect.height);
	grayThreshFar.allocate(kinect.width, kinect.height);
	
	nearThreshold = 230;
	farThreshold = 70;
    debugKinect = false;
    
}

//--------------------------------------------------------------
void ofApplication::update(){
    
    float elapsedPhase = 2.0*M_PI*ofGetElapsedTimef();
    
    // update kinect
    if (kinect.isConnected()){
        kinect.update();
        if (kinect.isFrameNew())
        {
            grayImage.setFromPixels(kinect.getDepthPixels(), kinect.width, kinect.height);        
            grayThreshNear = grayImage;
            grayThreshFar = grayImage;
            grayThreshNear.threshold(nearThreshold, true);
            grayThreshFar.threshold(farThreshold);
            cvAnd(grayThreshNear.getCvImage(), grayThreshFar.getCvImage(), grayImage.getCvImage(), NULL);
            grayImage.flagImageChanged();
            contourFinder.findContours(grayImage, 10, (kinect.width*kinect.height)/2, 2, false);
        }
    }
    
    
    // draw to FBO
    
    glDisable(GL_DEPTH_TEST);
    mainFbo.begin();
    ofFill();
    
    if (ofGetFrameNum() % 180 == 0)
    {
        blurVelocity = ofPoint(1.0f,1.0f)*ofRandom(100.0f, 300.0f);
        blurDirection = ofPoint(1.0f,1.0f)*ofRandom(-0.2f, 0.2f);
    }
    
    ofPoint scaledBlurVelocity = blurVelocity/10000.0f;
    ofPoint scaledBlurDirection = (ofPoint(0.5,0.5) + blurDirection) * scaledBlurVelocity;
    
    ofTexture & fadingTex = mainFbo.getTextureReference(1);

    mainFbo.setActiveDrawBuffer(0);
    ofSetColor(255,255,255);
    
    ofPushMatrix();
    ofScale(1.0f + scaledBlurVelocity.x, 1.0f + scaledBlurVelocity.y);
    
    // re center
    ofPoint translation = -(ofPoint(ofGetWidth(), ofGetHeight())*scaledBlurDirection);
    ofTranslate(translation);
    fadingTex.draw(0,0);
    ofPopMatrix();

#ifdef USE_MOUSE
    if (ofGetMousePressed()){
        
        ofPoint mousePoint = ofPoint(ofGetMouseX(), ofGetMouseY());
        mouseVelocity = fabs(mousePoint.distance(lastMousePoint))*100.0f/ofGetFrameRate();
        lastMousePoint = mousePoint;
        
        float saturation = ofMap(mouseVelocity, 0.0f, 100.0f, 180.0f, 255.0f, true);
        float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
        float radius = ofMap(smoothedVol, 0.0f, 0.25f, 2.0f, MAX_LINE_RADIUS, true);
        float angle = M_PI*2.0f*ofRandomf();
        
        ofSetColor(ofColor::fromHsb(hue, saturation, 255.0f));
        ofSetLineWidth(3.0f);
        ofPoint endPt = ofPoint(mousePoint.x + cosf(angle)*radius, mousePoint.y + sinf(angle)*radius);
        
        ofLine(lastEndPoint, endPt);
        lastEndPoint = endPt;
    }
#else
    
    vector<ofxCvBlob> & blobs = contourFinder.blobs;
    if (blobs.size()){
        
        ofPoint & centroid = blobs[0].centroid;
        
        float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
        float radius = ofMap(smoothedVol, 0.0f, 0.25f, 2.0f, MAX_LINE_RADIUS, true);
        float angle = M_PI*2.0f*ofRandomf();
        
        ofSetColor(ofColor::fromHsb(hue, 230, 255));
        ofSetLineWidth(3.0f);
        ofPoint endPt = ofPoint(centroid.x + cosf(angle)*radius, centroid.y + sinf(angle)*radius);
        
        ofLine(lastEndPoint, endPt);
        lastEndPoint = endPt;
    }
    
#endif
    
    ofTexture & mainTex = mainFbo.getTextureReference(0);

    mainFbo.setActiveDrawBuffer(1);
    ofSetColor(255,255,255);
    blurShader.begin();
    blurShader.setUniformTexture("texSampler", mainTex, 1);
    mainTex.draw(0,0);
    blurShader.end();
    
    mainFbo.end();
}

//--------------------------------------------------------------
void ofApplication::draw(){
    
    glDisable(GL_DEPTH_TEST);
    
    float bright = ofMap(smoothedVol, 0.0f, 0.3f, 20.0f, 160.0f, true);
    ofBackgroundGradient(ofColor::fromHsb(180, 80, bright), ofColor(0,0,0));
    ofSetColor(255, 255, 255);
    mainFbo.draw(0, 0);
//    blurShader.begin();
//    blurShader.setUniformTexture("texSampler", mainFbo.getTextureReference(), 1);
//    mainFbo.draw(0, 0);
//    blurShader.end();

}

#pragma mark - Audio 

void ofApplication::audioIn(float * input, int bufferSize, int nChannels)
{
    float curVol = 0.0;
	
	int numCounted = 0;
    
	// peak volume per buffer
	for (int i = 0; i < bufferSize; i++){
		left[i]		= input[i*2];
		right[i]	= input[i*2+1];
		curVol += left[i] * left[i];
		curVol += right[i] * right[i];
		numCounted++;
	}
	
	curVol /= (float)numCounted;
	
	smoothedVol *= 0.8;
	smoothedVol += 0.2 * curVol;
}

#pragma mark - Inputs

//--------------------------------------------------------------
void ofApplication::keyPressed(int key){
    switch (key) {
        case 'o':
            kinect.open();
            kinect.setCameraTiltAngle(0);
            break;
            
        case 'a':
			kinectAngle++;
			if(kinectAngle>30) kinectAngle=30;
			kinect.setCameraTiltAngle(kinectAngle);
			break;
			
		case 'z':
			kinectAngle--;
			if(kinectAngle<-30) kinectAngle=-30;
			kinect.setCameraTiltAngle(kinectAngle);
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