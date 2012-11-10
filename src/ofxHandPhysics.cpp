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
    handPositions.assign(MAX_POINT_HISTORY, ofPoint());
    spritePositions.assign(MAX_POINT_HISTORY, ofPoint());
    handVelocity = 0.0f;
    spriteVelocity = 0.0f;
    isNew = true;
}

ofxHandPhysicsManager::ofxHandPhysicsManager(ofxOpenNI &openNIDevice, bool useUserGenerator) :
    _openNIDevice(openNIDevice),
    _usingUserGenerator(useUserGenerator),
    _width(0.0f),
    _height(0.0f),
    physicsEnabled(false),
    spriteMass(1.0f),
    springCoef(100.0f),
    friction(0.05f),
    restDistance(10.0f),
    gravity(ofPoint(0, 1000.0f)),
    smoothCoef(0.66f)
{
    if (useUserGenerator){
        ofAddListener(openNIDevice.userEvent, this, &ofxHandPhysicsManager::userEvent);
    }
    else{
        ofAddListener(openNIDevice.handEvent, this, &ofxHandPhysicsManager::handEvent);
    }
}

ofxHandPhysicsManager::~ofxHandPhysicsManager()
{
    if (_usingUserGenerator){
        ofRemoveListener(_openNIDevice.userEvent, this, &ofxHandPhysicsManager::userEvent);
    }
    else{
        ofRemoveListener(_openNIDevice.handEvent, this, &ofxHandPhysicsManager::handEvent);
    }
}

void ofxHandPhysicsManager::update()
{
    // update physics settings, inserting new ones as necessary
    if (_usingUserGenerator){
        for (int th = 0; th < _trackedUserOrHandIDs.size(); th++)
        {
            XnUserID userID = _trackedUserOrHandIDs[th];
            ofPoint & handPositionL = _openNIDevice.getTrackedUser(th).getJoint(JOINT_LEFT_HAND).getProjectivePosition();
            ofxHandPhysicsState & physStateL = _trackedUserHandPhysicsL[userID];
            updatePhysState(physStateL, handPositionL);
            
            ofPoint & handPositionR = _openNIDevice.getTrackedUser(th).getJoint(JOINT_RIGHT_HAND).getProjectivePosition();
            ofxHandPhysicsState & physStateR = _trackedUserHandPhysicsR[userID];
            updatePhysState(physStateR, handPositionR);
        }

    }
    else{
        for (int th = 0; th < _trackedUserOrHandIDs.size(); th++)
        {
            XnUserID handId = _trackedUserOrHandIDs[th];
            ofPoint & handPosition = _openNIDevice.getHand(handId).getPosition();
            ofxHandPhysicsState & physState = _trackedHandPhysics[handId];
            updatePhysState(physState, handPosition);
        }
    }
}

unsigned int ofxHandPhysicsManager::getNumTrackedHands()
{
    return _usingUserGenerator ? _trackedUserOrHandIDs.size()*2 : _trackedUserOrHandIDs.size();
}

ofxHandPhysicsManager::ofxHandPhysicsState ofxHandPhysicsManager::getPhysicsStateForHand(unsigned int i)
{
    return handPhysicsForIndex(i);
}

ofPoint ofxHandPhysicsManager::getNormalizedSpritePositionForHand(unsigned int i, unsigned int stepIndex)
{
    if (_width == 0.0f || _height == 0.0f)
    {
        _width = _openNIDevice.getWidth();
        _height = _openNIDevice.getHeight();
    }
    
    // don't divide by zero
    if (_width == 0.0f || _height == 0.0f)
    {
        return ofPoint(0,0);
    }
    
    stepIndex = CLAMP(stepIndex, 0, MAX_POINT_HISTORY);
    
    ofPoint returnPoint = handPhysicsForIndex(i).spritePositions[stepIndex];
    return returnPoint * ofPoint(1.0f/_width, 1.0f/_height);
}

float ofxHandPhysicsManager::getAbsSpriteVelocityForHand(unsigned int i)
{
    return handPhysicsForIndex(i).spriteVelocity.length();
}

void ofxHandPhysicsManager::updatePhysState(ofxHandPhysicsManager::ofxHandPhysicsState &physState, ofPoint &handPosition)
{
    float currentTime = ofGetElapsedTimef();
    
    if (physState.isNew){
        physState.spritePositions.assign(MAX_POINT_HISTORY, handPosition);
        physState.handPositions.assign(MAX_POINT_HISTORY, handPosition);
        physState.isNew = false;
    }
    else{
        
        // update hand position
        handPosition *= 1.0f - smoothCoef;
        handPosition += physState.handPositions[0]*smoothCoef;
        
        physState.handPositions.pop_back();
        physState.handPositions.insert(physState.handPositions.begin(), handPosition);
        
        double dTime = currentTime - physState.lastUpdateTime;
        physState.handVelocity = (handPosition - physState.handPositions[0])/dTime;
        
        if (physicsEnabled){
            
            ofVec2f dStretch = physState.spritePositions[0] - physState.handPositions[0];
            dStretch -= restDistance*dStretch.getNormalized();
            
            ofVec2f force = -dStretch*springCoef;
            force += gravity*spriteMass;
            physState.spriteAcceleration = force/spriteMass;
            physState.spriteVelocity += physState.spriteAcceleration * dTime;
            physState.spriteVelocity *= 1.0f - friction;
            
            ofVec3f newPosition = physState.spritePositions[0] + (physState.spriteVelocity*dTime);
            physState.spritePositions.pop_back();
            physState.spritePositions.insert(physState.spritePositions.begin(), newPosition);
        }
        else{
            physState.spritePositions.pop_back();
            physState.spritePositions.insert(physState.spritePositions.begin(),  handPosition);
            physState.spriteVelocity = physState.handVelocity;
        }
        
    }
    
    physState.lastUpdateTime = currentTime;

}


void ofxHandPhysicsManager::userEvent(ofxOpenNIUserEvent &event)
{
    if (event.userStatus == USER_SKELETON_FOUND){
        _trackedUserOrHandIDs.push_back(event.id);
    }
    else if (event.userStatus == USER_SKELETON_LOST){
        
        _trackedUserHandPhysicsL.erase(event.id);
        _trackedUserHandPhysicsR.erase(event.id);
        
        vector<XnUserID>::iterator it = _trackedUserOrHandIDs.begin();
        while (it != _trackedUserOrHandIDs.end()){
            if (*it == event.id){
                _trackedUserOrHandIDs.erase(it);
                break;
            }
            it++;
        }
        
    }
}

void ofxHandPhysicsManager::handEvent(ofxOpenNIHandEvent & event)
{
    if (event.handStatus == HAND_TRACKING_STARTED){
        _trackedUserOrHandIDs.push_back(event.id);
    }
    else if (event.handStatus == HAND_TRACKING_STOPPED){
        
        _trackedHandPhysics.erase(event.id);
        
        vector<XnUserID>::iterator it = _trackedUserOrHandIDs.begin();
        while (it != _trackedUserOrHandIDs.end()){
            if (*it == event.id){
                _trackedUserOrHandIDs.erase(it);
                break;
            }
            it++;
        }
        
    }
}


ofxHandPhysicsManager::ofxHandPhysicsState & ofxHandPhysicsManager::handPhysicsForIndex(int index){
    if (_usingUserGenerator){
        
        if (index/2 >= _trackedUserOrHandIDs.size())
        {
            ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::handPhysicsForIndex - index out of bounds");
            return defaultPhysState;
        }
        
        
        int uIndex = index/2;
        int hIndex = index % 2;
        
        if (hIndex == 0){
            return _trackedUserHandPhysicsL[_trackedUserOrHandIDs[uIndex]];
        }
        else{
            return _trackedUserHandPhysicsR[_trackedUserOrHandIDs[uIndex]];
        }
    }
    else{
        
        if (index >= _trackedUserOrHandIDs.size())
        {
            ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::handPhysicsForIndex - index out of bounds");
            return defaultPhysState;
        }
        
        return _trackedHandPhysics[_trackedUserOrHandIDs[index]];
    }
    
}
