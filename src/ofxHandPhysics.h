//
//  ofxHandPhysics.h
//  drawAndFade
//
//  Created by Nick Donaldson on 10/29/12.
//
//

#pragma once

#include "ofMain.h"
#include "ofxOpenNI.h"
#include <map.h>

#define MAX_POINT_HISTORY   10

using std::map;

class ofxHandPhysicsManager {
    
public:
    
    struct ofxHandPhysicsState;
    
    ofxHandPhysicsManager(ofxOpenNI &openNIDevice, bool useUserGenerator = false);
    ~ofxHandPhysicsManager();
    
    void update();
    
    // state getters
    unsigned int getNumTrackedHands();
    
    ofxHandPhysicsState getPhysicsStateForHand(unsigned int i);
    ofPoint getNormalizedSpritePositionForHand(unsigned int i, unsigned int stepIndex = 0);
    float   getAbsSpriteVelocityForHand(unsigned int i);
    
    // input smoothing
    float       smoothCoef;
    
    // physics parameters
    bool        physicsEnabled;
    float       spriteMass;
    float       springCoef;
    float       restDistance;
    float       friction;
    ofVec2f     gravity;

    struct ofxHandPhysicsState {
        
        vector<ofPoint> handPositions;
        vector<ofPoint> spritePositions;
        ofVec2f handVelocity;
        ofVec2f spriteVelocity;
        ofVec2f spriteAcceleration;
        
        float  lastUpdateTime;
        
        bool    isNew;

        ofxHandPhysicsState();
        
    };
    
private:

    void updatePhysState(ofxHandPhysicsState &physState, ofPoint &handPosition);

    void userEvent(ofxOpenNIUserEvent & event);
    void handEvent(ofxOpenNIHandEvent & event);
    
    ofxHandPhysicsState & handPhysicsForIndex(int index);
    ofxHandPhysicsState defaultPhysState;
    
    float   _width;
    float   _height;
    
    ofxOpenNI &_openNIDevice;
    bool    _usingUserGenerator;
    
    vector<XnUserID>                            _trackedUserOrHandIDs;
    map<XnUserID, ofxHandPhysicsState>          _trackedHandPhysics;
    map<XnUserID, ofxHandPhysicsState>          _trackedUserHandPhysicsL;
    map<XnUserID, ofxHandPhysicsState>          _trackedUserHandPhysicsR;


};

