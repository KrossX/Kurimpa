/*  Kurimpa - OGL3 Video Plugin for Emulators
 *  Copyright (C) 2013  KrossX
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

union RGBA5551
{
	struct{u16 R:5, G:5, B:5, A:1;};
	u16 RAW;

	RGBA5551(u16 data) : RAW(data) {};
};

union RGBA8
{
	struct{u8 R, G, B, A;};
	u32 RAW;

	RGBA8(u32 data) : RAW(data) {};
};

union VDATA
{
	struct{ s32 x:11; s32 : 5; s32 y:11; s32 : 5;};
	u32 RAW;

	VDATA(u32 data) : RAW(data) {};
};

struct VEC3
{
	float R, G, B, A;

	VEC3(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 0.0f) :
		R(r), G(g), B(b), A(a) {};
};

struct vectk
{
	s16 x, y;
	u8 u, v; // texture x, y

	u32 c;   // Color
	u16 tex; // Texture page/clut

	// Set vertex data
	void SetV(u32 data, s16 offx, s16 offy)
	{
		VDATA z(data);
		x = z.x + offx;
		y = z.y + offy;
	}

	// Set texture data
	void SetT(u32 data)
	{
		Word z; z.U32 = data;
		tex = z.U16[1];
		u = z.U8[0];
		v = z.U8[1];
	}
};
