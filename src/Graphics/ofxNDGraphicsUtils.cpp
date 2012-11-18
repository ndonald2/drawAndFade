//
//  ofxNDGraphicsUtils.c
//  drawAndFade
//
//  Created by Nick Donaldson on 11/11/12.
//
//

#include "ofxNDGraphicsUtils.h"

static ofMesh _nd_cg_mesh;

void ofxBillboardRect(int x, int y, int w, int h, int tw, int th)
{
    GLfloat tex_coords[] = {
		0,0,
		tw,0,
		tw,th,
		0,th
	};
	GLfloat verts[] = {
		x,y,
		x+w,y,
		x+w,y+h,
		x,y+h
	};
	
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );
	glTexCoordPointer(2, GL_FLOAT, 0, tex_coords );
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(2, GL_FLOAT, 0, verts );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void ofxCircularGradient(const ofColor & start, const ofColor & end)
{
    int n = 32; // circular gradient resolution
    
    if (_nd_cg_mesh.getNumVertices() == 0){
        _nd_cg_mesh.setMode(OF_PRIMITIVE_TRIANGLE_FAN);
        ofVec2f center(0,0);
        _nd_cg_mesh.addVertex(center);

        float angleBisector = TWO_PI / (n * 2);
        float smallRadius = 1.0f;
        float bigRadius = smallRadius / cos(angleBisector);
        for(int i = 0; i <= n; i++) {
            float theta = i * TWO_PI / n;
            _nd_cg_mesh.addVertex(center + ofVec2f(sin(theta), cos(theta)) * bigRadius);
        }
    }
    
    _nd_cg_mesh.clearColors();
    _nd_cg_mesh.addColor(start);
    for(int i = 0; i <= n; i++) {
        _nd_cg_mesh.addColor(end);
    }
    
    _nd_cg_mesh.draw();
}