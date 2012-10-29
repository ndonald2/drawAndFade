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
    
    ofxHandPhysicsManager(ofxOpenNI &openNIDevice);
    
    void update();
    
    // state getters
    unsigned int getNumTrackedHands();
    
    ofxHandPhysicsState getPhysicsStateForHand(unsigned int i);
    ofPoint getNormalizedPositionForHand(unsigned int i, unsigned int stepIndex = 0);
    float getAbsVelocityOfHand(unsigned int i);
    
    // physics setters
    void setPhysicsEnabled(bool enabled);
    void setSpringPhysics(float mass, float coef, float friction);
    
    struct ofxHandPhysicsState {
        
        vector<ofPoint> affectedHandPositions;
        vector<ofPoint> handPositions;
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

