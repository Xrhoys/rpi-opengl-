/* date = July 24th 2023 0:41 am */

#ifndef MATH_H
#define MATH_H

// NOTE(Xrhoys): only here for trigo/log functions, will be replaced later with approximations
#include <math.h>

union v4
{
	struct
	{
		r32 r, g, b, a;
	};
	
	struct
	{
		r32 x, y, z, w;
	};
	
	r32 _E[4];
	
	r32 operator[](u32 index);
};

struct v3
{
	struct
	{
		r32 x, y, z;
	};
	
	r32 _E[3];
	
	r32 operator[](u32 index);
};

union v2
{
	struct
	{
		r32 u, v;
	};
	
	struct
	{
		r32 x, y;
	};
	
	r32 _E[2];
	
	r32 operator[](u32 index);
};

union v2U32
{
	struct
	{
		u32 x, y;
	};
	
	struct
	{
		u32 width, height;
	};
	
	struct
	{
		u32 left, top;
	};
	
	u32 _E[2];
};

inline r32
v4::operator[](u32 index)
{
	return _E[index];
}

inline r32
v3::operator[](u32 index)
{
	return _E[index];
}

inline r32
v2::operator[](u32 index)
{
	return _E[index];
}

#endif //MATH_H
