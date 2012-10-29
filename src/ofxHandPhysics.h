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

using std::map;

class ofxHandPhysicsManager {
    
public:
    
    struct ofxHandPhysicsState;
    
    ofxHandPhysicsManager(ofxOpenNI &openNIDevice);
    
    void update();
    
    // state getters
    unsigned int getNumTrackedHands();
    
    ofxHandPhysicsState getPhysicsStateForHand(unsigned int i);
    ofPoint getNormalizedPositionForHand(unsigned int i);
    float getAbsVelocityOfHand(unsigned int i);
    
    // physics setters
    void setPhysicsEnabled(bool enabled);
    void setSpringPhysics(float mass, float coef, float friction);
    
    struct ofxHandPhysicsState {
        
        ofPoint affectedHandPosition;
        ofPoint handPosition;
        ofVec3f velocity;
        ofVec3f affectedVelocity;
        
        double  lastUpdateTime;
        
        bool    isNew;

        ofxHandPhysicsState();
        
    };
    
private:

    float   _width;
    float   _height;
    
    ofxOpenNI &_openNIDevice;
    
    vector<XnUserID>                     _trackedHandIDs;
    map<XnUserID, ofxHandPhysicsState>   _trackedHandPhysics;
    
    bool    _physicsEnabled;
    float   _mass;
    float   _springCoef;
    float   _friction;
};

