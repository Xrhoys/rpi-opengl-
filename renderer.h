/* date = July 9th 2023 2:21 am */

#ifndef RENDERER_H
#define RENDERER_H

#include "asset_build.h"
#include "utils.h"

// Custom raylib color palette for amazing visuals on WHITE background
#define LIGHTGRAY  { 200, 200, 200, 255 }   // Light Gray
#define GRAY       { 130, 130, 130, 255 }   // Gray
#define DARKGRAY   {  80,  80,  80, 255 }   // Dark Gray
#define YELLOW     { 253, 249,   0, 255 }   // Yellow
#define GOLD       { 255, 203,   0, 255 }   // Gold
#define ORANGE     { 255, 161,   0, 255 }   // Orange
#define PINK       { 255, 109, 194, 255 }   // Pink
#define RED        { 230,  41,  55, 255 }   // Red
#define MAROON     { 190,  33,  55, 255 }   // Maroon
#define GREEN      {   0, 228,  48, 255 }   // Green
#define LIME       {   0, 158,  47, 255 }   // Lime
#define DARKGREEN  {   0, 117,  44, 255 }   // Dark Green
#define SKYBLUE    { 102, 191, 255, 255 }   // Sky Blue
#define BLUE       {   0, 121, 241, 255 }   // Blue
#define DARKBLUE   {   0,  82, 172, 255 }   // Dark Blue
#define PURPLE     { 200, 122, 255, 255 }   // Purple
#define VIOLET     { 135,  60, 190, 255 }   // Violet
#define DARKPURPLE { 112,  31, 126, 255 }   // Dark Purple
#define BEIGE      { 211, 176, 131, 255 }   // Beige
#define BROWN      { 127, 106,  79, 255 }   // Brown
#define DARKBROWN  {  76,  63,  47, 255 }   // Dark Brown

#define WHITE      { 255, 255, 255, 255 }   // White
#define BLACK      {   0,   0,   0, 255 }   // Black
#define BLANK      {   0,   0,   0,   0 }   // Blank (Transparent)
#define MAGENTA    { 255,   0, 255, 255 }   // Magenta
#define RAYWHITE   { 245, 245, 245, 255 }   // My own White (raylib logo)

union vertex
{
	struct 
	{
		r32 pos[3];
		r32 texCoord[2];
		r32  color[4];	
	};
	
	struct 
	{
		r32 x, y, z;
		r32 u, v;
		r32  r, g, b, a;
	};
};

struct render_group
{
	app_state *appContext;
	u32      mode;
	
	vertex   *vertices;
	u32      vertexCount;
	
	u32      *indices;
	u32      indexCount;
	
	memory_arena *arena;
};

union color
{
	struct
	{
		u8 r, g, b, a;
	};
	
	u8 E[4];
};

inline vertex
Vertex(r32 x, r32 y, r32 z, r32 u, r32 v, r32 r, r32 g, r32 b, r32 a)
{
	vertex vx;
	vx.x = x;
	vx.y = y;
	vx.z = z;
	
	vx.u = u;
	vx.v = v;
	
	vx.r = r;
	vx.g = g;
	vx.b = b;
	vx.a = a;
	
	return vx;
}

enum buffer_labels
{
	BG_VERTEX_ARRAY,
	BG_INDEX_ARRAY,
	FONT_VERTEX_ARRAY,
	FONT_INDEX_ARRAY,
	UI_VERTEX_ARRAY,
	UI_INDEX_ARRAY,
	
	BUFFER_COUNT,
};

// TODO(Ecy): either generate float versions of color macros
// OR do the transform shader side. Having byte sized color channel is useful for making tooling later
inline v4
RGBToFloat(color cl)
{
	v4 vec;
	vec.r = cl.r * 0.00390625f;
	vec.g = cl.g * 0.00390625f;
	vec.b = cl.b * 0.00390625f;
	vec.a = cl.a * 0.00390625f;
	
	return vec;
}

inline render_group
CreateRenderGroup(app_state *appContext, memory_arena *arena, u32 vArraySize, u32 iArraySize)
{
	render_group render;
	
	render.appContext = appContext;
	render.arena = arena;
	
	render.vertices = (vertex*)LinearAlloc(arena, vArraySize);
	render.indices  = (u32*)LinearAlloc(arena, iArraySize);
	
	return render;
};

// NOTE(Ecy): not sure if this should be kept or not, to remove if not used at all
inline void
PushAxisAlignedRect(render_group *group, r32 x, r32 y, r32 width, r32 height, r32 *color)
{
	Assert(group->appContext);
	
	r32 posX = (2.0f * x / group->appContext->width) - 1.0f;
	r32 posY = (-2.0f * y / group->appContext->height) + 1.0f ;
	
	r32 posWidth = (2.0f * (x + width) / group->appContext->width) - 1.0f;
	r32 posHeight = (-2.0f * (y + height) / group->appContext->height) + 1.0f;
	
	vertex *v = &group->vertices[group->vertexCount];
	u32 *i = &group->indices[group->indexCount];
	
	v[0] = Vertex(posX,     posHeight, 0.0f,  0.0f, 1.0f, color[0], color[1], color[2], color[3]);
	v[1] = Vertex(posWidth, posHeight, 0.0f,  1.0f, 1.0f, color[0], color[1], color[2], color[3]);
	v[2] = Vertex(posWidth, posY,      0.0f,  1.0f, 0.0f, color[0], color[1], color[2], color[3]);
	v[3] = Vertex(posX,     posY,      0.0f,  0.0f, 0.0f, color[0], color[1], color[2], color[3]);
	
	i[0] = group->vertexCount;
	i[1] = group->vertexCount + 1;
	i[2] = group->vertexCount + 3;
	i[3] = group->vertexCount + 1;
	i[4] = group->vertexCount + 2;
	i[5] = group->vertexCount + 3;
	
	group->vertexCount += 4;
	group->indexCount += 6;
}

// TODO(Ecy): look to merge this to the glyph version, since it's really not that different
inline void
PushAxisAlignedGlyph(render_group *group, r32 x, r32 y, r32 width, r32 height, 
					 r32 u, r32 v, r32 u2, r32 v2, r32 *color)
{
	Assert(group->appContext);
	// TODO(Ecy): remove this global state later
	r32 posX = (2.0f * x / group->appContext->width) - 1.0f;
	r32 posY = (-2.0f * y / group->appContext->height) + 1.0f ;
	
	r32 posWidth = (2.0f * (x + width) / group->appContext->width) - 1.0f;
	r32 posHeight = (-2.0f * (y + height) / group->appContext->height) + 1.0f;
	
	vertex *vt = &group->vertices[group->vertexCount];
	u32 *i = &group->indices[group->indexCount];
	
	/*
		A--B  A(x,y)                       (u, v)                     
		|\ |  B(x + width, y)              (u + glyphWidth, v)
		| \|  C(x + width, y + height)     (u + glyphWidth, v + glyphHeight)
		D--C  D(x, y + height)             (u, v + glyphHeight)

		NOTE(Ecy): the following is upside down due to the screen orientation convention used
	*/
	
	vt[0] = Vertex(posX,     posHeight, 0.5f,   u, v2, color[0], color[1], color[2], color[3]);
	vt[1] = Vertex(posWidth, posHeight, 0.5f,  u2, v2, color[0], color[1], color[2], color[3]);
	vt[2] = Vertex(posWidth, posY,      0.5f,  u2,  v, color[0], color[1], color[2], color[3]);
	vt[3] = Vertex(posX,     posY,      0.5f,   u,  v, color[0], color[1], color[2], color[3]);
	
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
