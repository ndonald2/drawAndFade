//
//  ofxNDGraphicsUtils.h
//  drawAndFade
//
//  Created by Nick Donaldson on 11/11/12.
//
//

#pragma once

#include "ofMain.h"

// Billboard rectangle - rect for displaying a texture. Draws vertices and ARB tex coords.
extern void ofxBillboardRect(int x, int y, int w, int h, int tw, int th);



// Circular gradient
extern void ofxCircularGradient(const ofColor & start, const ofColor & end);