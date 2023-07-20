/* date = July 9th 2023 2:21 am */

#ifndef RENDERER_H
#define RENDERER_H

#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <GL/gl.h>
#include <GLES2/gl2.h>
#include <GLES3/gl3.h>

#define GL_ES_VERSION_3_0 1


#define STB_TRUETYPE_IMPLEMENTATION 1
#include "stb_truetype.h"

#include "asset_build.h"

struct vertex
{
	r32 x, y, z, u, v;
};

 struct render_group
{
	u32      mode;
	
	vertex   *vertices;
	u32      vertexCount;
	
	u32      *indices;
	u32      indexCount;
};

inline vertex
Vertex(r32 x, r32 y, r32 z, r32 u, r32 v)
{
	vertex vx;
	vx.x = x;
	vx.y = y;
	vx.z = z;
	
	vx.u = u;
	vx.v = v;
	
	return vx;
}

// NOTE(Ecy): not sure if this should be kept or not, to remove if not used at all
inline void
PushAxisAlignedRect(render_group *group, app_state *state,
					u32 x, u32 y, u32 width, u32 height)
{
	r32 posX = (2.0f * x / state->width) - 1.0f;
	r32 posY = (-2.0f * y / state->height) + 1.0f ;
	
	r32 posWidth = (2.0f * (x + width) / state->width) - 1.0f;
	r32 posHeight = (-2.0f * (y + height) / state->height) + 1.0f;
	
	vertex *v = &group->vertices[group->vertexCount];
	u32 *i = &group->indices[group->indexCount];
	
	v[0] = Vertex(posX,     posHeight, 0.0f,  0.0f, 1.0f);
	v[1] = Vertex(posWidth, posHeight, 0.0f,  1.0f, 1.0f);
	v[2] = Vertex(posWidth, posY,      0.0f,  1.0f, 0.0f);
	v[3] = Vertex(posX,     posY,      0.0f,  0.0f, 0.0f);
	
	i[0] = group->vertexCount;
	i[1] = group->vertexCount + 1;
	i[2] = group->vertexCount + 3;
	i[3] = group->vertexCount + 1;
	i[4] = group->vertexCount + 2;
	i[5] = group->vertexCount + 3;
	
	group->vertexCount += 4;
	group->indexCount += 6;
}

inline void
PushAxisAlignedGlyph(render_group *group, app_state *state,
					 u32 x, u32 y, r32 width, r32 height, 
					 r32 u, r32 v, u32 glyphWidth, u32 glyphHeight)
{
	// TODO(Ecy): remove this global state later
	r32 posX = (2.0f * x / state->width) - 1.0f;
	r32 posY = (-2.0f * y / state->height) + 1.0f ;
	
	r32 posWidth = (2.0f * (x + width) / state->width) - 1.0f;
	r32 posHeight = (-2.0f * (y + height) / state->height) + 1.0f;
	
	vertex *vt = &group->vertices[group->vertexCount];
	u32 *i = &group->indices[group->indexCount];
	
	/*
		A--B  A(x,y)                       (u, v)                     
		|\ |  B(x + width, y)              (u + glyphWidth, v)
		| \|  C(x + width, y + height)     (u + glyphWidth, v + glyphHeight)
		D--C  D(x, y + height)             (u, v + glyphHeight)

		NOTE(Ecy): the following is upside down due to the screen orientation convention used
	*/
	
	vt[0] = Vertex(posX,     posHeight, 0.0f,  u, v + glyphHeight);
	vt[1] = Vertex(posWidth, posHeight, 0.0f,  u + glyphWidth, v + glyphHeight);
	vt[2] = Vertex(posWidth, posY,      0.0f,  u + glyphWidth, v);
	vt[3] = Vertex(posX,     posY,      0.0f,  u, v);
	
	i[0] = group->vertexCount;
	i[1] = group->vertexCount + 1;
	i[2] = group->vertexCount + 3;
	i[3] = group->vertexCount + 1;
	i[4] = group->vertexCount + 2;
	i[5] = group->vertexCount + 3;
	
	group->vertexCount += 4;
	group->indexCount += 6;
}

#endif //RENDERER_H
