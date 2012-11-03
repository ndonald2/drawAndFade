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

ofxHandPhysicsManager::ofxHandPhysicsManager(ofxOpenNI &openNIDevice) :
    _openNIDevice(openNIDevice),
    physicsEnabled(false),
    spriteMass(1.0f),
    springCoef(100.0f),
    friction(0.05f),
    restDistance(10.0f),
    gravity(ofPoint(0, 1000.0f)),
    smoothCoef(0.66f)
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
        return ofxHandPhysicsState(); // yeah, yeah, warning...
    }
    
    XnUserID thId = _trackedHandIDs[i];
    return _trackedHandPhysics[thId];
}

ofPoint ofxHandPhysicsManager::getNormalizedSpritePositionForHand(unsigned int i, unsigned int stepIndex)
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
    ofPoint returnPoint = _trackedHandPhysics[thId].spritePositions[stepIndex];
    return returnPoint * ofPoint(1.0f/_width, 1.0f/_height);
}

float ofxHandPhysicsManager::getAbsSpriteVelocityForHand(unsigned int i)
{
    if (i >= _trackedHandIDs.size())
    {
        ofLog(OF_LOG_ERROR, "ofxHandPhysicsManager::getNormalizedPositionForHand - index out of bounds");
        return 0.0f;
    }
    
    XnUserID thId = _trackedHandIDs[i];
    return _trackedHandPhysics[thId].spriteVelocity.length();
}


