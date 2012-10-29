//
//  ofxHandPhysics.cpp
//  drawAndFade
//
//  Created by Nick Donaldson on 10/29/12.
//
//

#include "ofxHandPhysics.h"
#include <algorithm>

ofxHandPhysicsManager::ofxHandPhysicsState::ofxHandPhysicsState()
{
    velocity = 0.0f;
    affectedVelocity = 0.0f;
    isNew = true;
}

ofxHandPhysicsManager::ofxHandPhysicsManager(ofxOpenNI &openNIDevice) :
    _openNIDevice(openNIDevice),
    _physicsEnabled(false),
    _mass(0.0f),
    _springCoef(0.0f),
    _friction(0.0f)
{
    _width = openNIDevice.getWidth();
    _height = openNIDevice.getHeight();
}

void ofxHandPhysicsManager::update()
{
    set<XnUserID> activeHandIDs;
    set<XnUserID> deadHandIDs;
    set<XnUserID>::iterator it;
    
    // get current tracked hand IDs

    for (int i=0; i<_openNIDevice.getNumTrackedHands(); i++)
    {
        activeHandIDs.insert(_openNIDevice.getTrackedHand(i).getID());
    }
        
    // get no longer tracked hand IDs
    for (int th=0; th < _trackedHandIDs.size(); th++)
    {
        if (activeHandIDs.find(_trackedHandIDs[th]) == activeHandIDs.end())
        {
            deadHandIDs.insert(_trackedHandIDs[th]);
        }
    }
    
    // remove physics settings for invalid hand id's
    it = deadHandIDs.begin();
    while (it != deadHandIDs.end()){
        _trackedHandPhysics.erase(*it++);
    }
    
    // setup new tracked hand IDs
    _trackedHandIDs.clear();
    it = activeHandIDs.begin();
    while (it != activeHandIDs.end())
    {
        _trackedHandIDs.push_back(*it++);
    }
    
    // update physics settings, inserting new ones as necessary
    double currentTime = (double)ofGetSystemTime()/1000.0;
    for (int th = 0; th < _trackedHandIDs.size(); th++)
    {
        XnUserID handId = _trackedHandIDs[th];
        ofPoint & handPosition = _openNIDevice.getHand(handId).getPosition();
        ofxHandPhysicsState & physState = _trackedHandPhysics[handId];
        
        if (physState.isNew){
            physState.affectedHandPosition = handPosition;
            physState.isNew = false;
        }
        else{
            double dTime = currentTime - physState.lastUpdateTime;
            physState.velocity = (handPosition - physState.handPosition)/dTime;
            
            if (_physicsEnabled){
                
                
            }
            else{
                physState.affectedHandPosition = handPosition;
                physState.affectedVelocity = physState.velocity;
            }
        }
        
        physState.handPosition = handPosition;
        physState.lastUpdateTime = currentTime;
    }
}

unsigned int ofxHandPhysicsManager::getNumTrackedHands()
{
    return _trackedHandIDs.size();
}

ofxHandPhysicsManager::ofxHandPhysicsState ofxHandPhysicsManager::getPhysicsStateForHand(unsigned int i)
{
    if (i >= _trackedHandIDs.size())
    {
        ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::getPhysicsStateForHand - index out of bounds");
        return ofxHandPhysicsState();
    }
    
    return _trackedHandPhysics[_trackedHandIDs[i]];
}

ofPoint ofxHandPhysicsManager::getNormalizedPositionForHand(unsigned int i)
{
    if (i >= _trackedHandIDs.size())
    {
        ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::getNormalizedPositionForHand - index out of bounds");
        return ofPoint();
    }
    ofPoint returnPoint = _trackedHandPhysics[i].affectedHandPosition;
    return returnPoint * ofPoint(1.0f/_width, 1.0f/_height);
}

float ofxHandPhysicsManager::getAbsVelocityOfHand(unsigned int i)
{
    if (i >= _trackedHandIDs.size())
    {
        ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::getNormalizedPositionForHand - index out of bounds");
        return 0.0f;
    }
    
    return _trackedHandPhysics[_trackedHandIDs[i]].affectedVelocity.length();
}

void ofxHandPhysicsManager::setPhysicsEnabled(bool enabled){
    _physicsEnabled = enabled;
}

void ofxHandPhysicsManager::setSpringPhysics(float mass, float coef, float friction)
{
    _mass = mass;
    _springCoef = coef;
    _friction = friction;
}


