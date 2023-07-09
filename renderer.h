/* date = July 9th 2023 2:21 am */

#ifndef RENDERER_H
#define RENDERER_H

#include "main.h"

typedef struct vertex
{
	r32 x, y, z, u, v;
} vertex;

typedef struct render_group
{
	u32      mode;
	
	vertex   *vertices;
	u32      vertexCount;
	
	u32      *indices;
	u32      indexCount;
} render_group;

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

inline void
DebugRenderText(render_group *group, app_state *appState, char *buffer,
				u32 size, u32 x, u32 y, u32 scale)
{
	for(u32 index = 0;
		index < size;
		++index)
	{
		char currentCharacter = buffer[index];
		
		// TODO(Ecy): load uv font coords from assets
		r32 u = 0.0f;
		r32 v = 0.0f;
		r32 glyphWidth = 1.0f;
		r32 glyphHeight = 1.0f;
		
		// TODO(Ecy): this ratio is queried from assets
		r32 ratio = 1.5;
		
		// NOTE(Ecy): what would be the scale of the font?
		PushAxisAlignedGlyph(group, appState, x + index * 100, y, scale, scale * ratio, u, v, glyphWidth, glyphHeight);
	}
}

#endif //RENDERER_H
