//
//  ofxNDGraphicsUtils.c
//  drawAndFade
//
//  Created by Nick Donaldson on 11/11/12.
//
//

#include "ofxNDGraphicsUtils.h"

void drawBillboardRect(int x, int y, int w, int h, int tw, int th)
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