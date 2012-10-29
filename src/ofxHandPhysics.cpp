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
    affectedHandPositions.assign(MAX_POINT_HISTORY, ofPoint());
    handPositions.assign(MAX_POINT_HISTORY, ofPoint());
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
            physState.affectedHandPositions.assign(MAX_POINT_HISTORY, handPosition);
            physState.handPositions.assign(MAX_POINT_HISTORY, handPosition);
            physState.isNew = false;
        }
        else{
            double dTime = currentTime - physState.lastUpdateTime;
            physState.velocity = (handPosition - physState.handPositions[0])/dTime;
            
            if (_physicsEnabled){
                
                
            }
            else{
                physState.affectedHandPositions.pop_back();
                physState.affectedHandPositions.insert(physState.affectedHandPositions.begin(),  handPosition);
                physState.affectedVelocity = physState.velocity;
            }
            
            physState.handPositions.pop_back();
            physState.handPositions.insert(physState.handPositions.begin(), handPosition);
        }

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
    
    XnUserID thId = _trackedHandIDs[i];
    return _trackedHandPhysics[thId];
}

ofPoint ofxHandPhysicsManager::getNormalizedPositionForHand(unsigned int i, unsigned int stepIndex)
{
    if (i >= _trackedHandIDs.size())
    {
        ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::getNormalizedPositionForHand - index out of bounds");
        return ofPoint();
    }
    
    if (_width == 0.0f || _height == 0.0f)
    {
        _width = _openNIDevice.getWidth();
        _height = _openNIDevice.getHeight();
    }
    
    stepIndex = CLAMP(stepIndex, 0, MAX_POINT_HISTORY);
    
    XnUserID thId = _trackedHandIDs[i];
    ofPoint returnPoint = _trackedHandPhysics[thId].affectedHandPositions[stepIndex];
    return returnPoint * ofPoint(1.0f/_width, 1.0f/_height);
}

float ofxHandPhysicsManager::getAbsVelocityOfHand(unsigned int i)
{
    if (i >= _trackedHandIDs.size())
    {
        ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::getNormalizedPositionForHand - index out of bounds");
        return 0.0f;
    }
    
    XnUserID thId = _trackedHandIDs[i];
    return _trackedHandPhysics[thId].affectedVelocity.length();
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


