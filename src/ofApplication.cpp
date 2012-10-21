#include "ofApplication.h"

#define MAX_BLOTCH_RADIUS_FACTOR    0.2f

static int inputDeviceId = 0;

void ofApplicationSetAudioInputDeviceId(int deviceId){
    inputDeviceId = deviceId;
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
    ofxAudioAnalyzer::Settings audioSettings;
    audioSettings.stereo = true;
    audioSettings.inputDeviceId = inputDeviceId;
    audioSettings.bufferSize = 512;
    audioAnalyzer.setup(audioSettings);
    
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
    ofSetColor(ofColor(255,255,255));
    
    ofPushMatrix();
    ofScale(1.0f + scaledBlurVelocity.x, 1.0f + scaledBlurVelocity.y);
    
    // re center
    ofPoint translation = -(ofPoint(ofGetWidth(), ofGetHeight())*scaledBlurDirection);
    ofTranslate(translation);
    fadingTex.draw(0,0);
    ofPopMatrix();

#ifdef USE_MOUSE
    if (ofGetMousePressed()){
        
        float highEnergy = audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH)*100.0f;

        ofPoint mousePoint = ofPoint(ofGetMouseX(), ofGetMouseY());
        mouseVelocity = fabs(mousePoint.distance(lastMousePoint))*100.0f/ofGetFrameRate();
        lastMousePoint = mousePoint;
        
        float saturation = ofMap(mouseVelocity, 0.0f, 100.0f, 255.0f, 180.0f, true);
        float hue = ((cosf(0.05f*elapsedPhase)+1.0f)/2.0f)*255.0f;
        float radius = ofMap(highEnergy, 0.2f, 1.0f, 0.0f, (float)ofGetWidth()*MAX_BLOTCH_RADIUS_FACTOR, true);
        
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
    
    float lowPSF = audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_LOW)*10.0f;
    float bright = ofMap(lowPSF, 0.0f, 1.0f, 20.0f, 160.0f, true);
    ofBackgroundGradient(ofColor::fromHsb(180, 80, bright), ofColor::fromHsb(0, 0, 20));
    ofSetColor(255, 255, 255);
    mainFbo.draw(0, 0);
    
//    stringstream ss;
//    ss << "Audio Signal Energy: " << audioAnalyzer.getSignalEnergy();
//    ofDrawBitmapString(ss.str(), 20,30);
//    ss.str(std::string());
//    ss << "Audio Signal PSF: " << audioAnalyzer.getTotalPSF();
//    ofDrawBitmapString(ss.str(), 20,45);
//    ss.str(std::string());
//    ss << "Region Energy -- Low: " << audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_LOW) <<
//    " Mid: " << audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_MID) << " High: " << audioAnalyzer.getSignalEnergyInRegion(AA_FREQ_REGION_HIGH);
//    ofDrawBitmapString(ss.str(), 20, 60);
//    ss.str(std::string());
//    ss << "Region PSF -- Low: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_LOW) <<
//    " Mid: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_MID) << " High: " << audioAnalyzer.getPSFinRegion(AA_FREQ_REGION_HIGH);
//    ofDrawBitmapString(ss.str(), 20, 75);

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