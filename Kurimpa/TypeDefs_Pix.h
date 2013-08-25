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

union RGBA5551 { struct{u16 R:5, G:5, B:5, A:1;}; u16 RAW; };
union RGBA8 { struct{u8 R, G, B, A;}; u32 RAW; };
union VDATA { struct{ s32 x:11; s32 : 5; s32 y:11; s32 : 5;}; u32 RAW;};

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
	void SetV(u32 data)
	{
		VDATA z; z.RAW = data;
		x = z.x;
		y = z.y;
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


struct Pixel32
{
	RGBA8 pix;

	Pixel32(u8 R = 0, u8 G = 0, u8 B = 0, u8 A = 0)
	{
		pix.R = R; pix.G = G; pix.B = B; pix.A = A;
	}

	Pixel32(u8 k)
	{
		pix.R = pix.G = pix.B = pix.A = k;
	}

	Pixel32(u32 RAW)
	{
		pix.RAW = RAW;
	}

	void operator = (const Pixel32 &in)
	{
		pix.RAW = in.pix.RAW;
	}

	void operator += (const Pixel32 &in)
	{
		pix.R += in.pix.R;
		pix.G += in.pix.G;
		pix.B += in.pix.B;
		pix.A += in.pix.A;
	}

	void operator -= (const Pixel32 &in)
	{
		pix.R -= in.pix.R;
		pix.G -= in.pix.G;
		pix.B -= in.pix.B;
		pix.A -= in.pix.A;
	}

	void operator *= (const Pixel32 &in)
	{
		pix.R *= in.pix.R;
		pix.G *= in.pix.G;
		pix.B *= in.pix.B;
		pix.A *= in.pix.A;
	}

	void operator /= (const Pixel32 &in)
	{
		pix.R = in.pix.R? pix.R / in.pix.R : 0;
		pix.G = in.pix.G? pix.G / in.pix.G : 0;
		pix.B = in.pix.B? pix.B / in.pix.B : 0;
		pix.A = in.pix.A? pix.A / in.pix.A : 0;
	}

	Pixel32 operator + (const Pixel32 &in) const
	{
		Pixel32 out(pix.RAW); out += in;
		return out;
	}

	Pixel32 operator - (const Pixel32 &in) const
	{
		Pixel32 out(pix.RAW); out -= in;
		return out;
	}

	Pixel32 operator * (const Pixel32 &in) const
	{
		Pixel32 out(pix.RAW); out *= in;
		return out;
	}

	Pixel32 operator / (const Pixel32 &in) const
	{
		Pixel32 out(pix.RAW); out /= in;
		return out;
	}

	// Scalar ops
	void operator = (u8 &in)
	{
		pix.R = pix.G = pix.B = pix.A = in;
	}

	void operator += (const u8 &in)
	{
		pix.R += in;
		pix.G += in;
		pix.B += in;
		pix.A += in;
	}

	void operator -= (const u8 &in)
	{
		pix.R -= in;
		pix.G -= in;
		pix.B -= in;
		pix.A -= in;
	}

	void operator *= (const u8 &in)
	{
		pix.R *= in;
		pix.G *= in;
		pix.B *= in;
		pix.A *= in;
	}

	void operator /= (const u8 &in)
	{
		if(in)
		{
			pix.R /= in;
			pix.G /= in;
			pix.B /= in;
			pix.A /= in;
		}
		else
			pix.RAW = 0;
	}

	Pixel32 operator + (const u8 &in) const
	{
		Pixel32 out(pix.RAW); out += in;
		return out;
	}

	Pixel32 operator - (const u8 &in) const
	{
		Pixel32 out(pix.RAW); out -= in;
		return out;
	}

	Pixel32 operator * (const u8 &in) const
	{
		Pixel32 out(pix.RAW); out *= in;
		return out;
	}

	Pixel32 operator / (const u8 &in) const
	{
		Pixel32 out(pix.RAW); out /= in;
		return out;
	}
};


// Yay for duplicated code!

struct Pixel16
{
	RGBA5551 pix;

	Pixel16(u8 R = 0, u8 G = 0, u8 B = 0, u8 A = 0)
	{
		pix.R = R; pix.G = G; pix.B = B; pix.A = A;
	}

	Pixel16(u8 k)
	{
		pix.R = pix.G = pix.B = pix.A = k;
	}

	Pixel16(u16 RAW)
	{
		pix.RAW = RAW;
	}

	void operator = (const Pixel16 &in)
	{
		pix.RAW = in.pix.RAW;
	}

	void operator += (const Pixel16 &in)
	{
		pix.R += in.pix.R;
		pix.G += in.pix.G;
		pix.B += in.pix.B;
		pix.A += in.pix.A;
	}

	void operator -= (const Pixel16 &in)
	{
		pix.R -= in.pix.R;
		pix.G -= in.pix.G;
		pix.B -= in.pix.B;
		pix.A -= in.pix.A;
	}

	void operator *= (const Pixel16 &in)
	{
		pix.R *= in.pix.R;
		pix.G *= in.pix.G;
		pix.B *= in.pix.B;
		pix.A *= in.pix.A;
	}

	void operator /= (const Pixel16 &in)
	{
		pix.R = in.pix.R? pix.R / in.pix.R : 0;
		pix.G = in.pix.G? pix.G / in.pix.G : 0;
		pix.B = in.pix.B? pix.B / in.pix.B : 0;
		pix.A = in.pix.A? pix.A / in.pix.A : 0;
	}

	Pixel16 operator + (const Pixel16 &in) const
	{
		Pixel16 out(pix.RAW); out += in;
		return out;
	}

	Pixel16 operator - (const Pixel16 &in) const
	{
		Pixel16 out(pix.RAW); out -= in;
		return out;
	}

	Pixel16 operator * (const Pixel16 &in) const
	{
		Pixel16 out(pix.RAW); out *= in;
		return out;
	}

	Pixel16 operator / (const Pixel16 &in) const
	{
		Pixel16 out(pix.RAW); out /= in;
		return out;
	}

	// Scalar ops
	void operator = (u8 &in)
	{
		pix.R = pix.G = pix.B = pix.A = in;
	}

	void operator += (const u8 &in)
	{
		pix.R += in;
		pix.G += in;
		pix.B += in;
		pix.A += in;
	}

	void operator -= (const u8 &in)
	{
		pix.R -= in;
		pix.G -= in;
		pix.B -= in;
		pix.A -= in;
	}

	void operator *= (const u8 &in)
	{
		pix.R *= in;
		pix.G *= in;
		pix.B *= in;
		pix.A *= in;
	}

	void operator /= (const u8 &in)
	{
		if(in)
		{
			pix.R /= in;
			pix.G /= in;
			pix.B /= in;
			pix.A /= in;
		}
		else
			pix.RAW = 0;
	}

	Pixel16 operator + (const u8 &in) const
	{
		Pixel16 out(pix.RAW); out += in;
		return out;
	}

	Pixel16 operator - (const u8 &in) const
	{
		Pixel16 out(pix.RAW); out -= in;
		return out;
	}

	Pixel16 operator * (const u8 &in) const
	{
		Pixel16 out(pix.RAW); out *= in;
		return out;
	}

	Pixel16 operator / (const u8 &in) const
	{
		Pixel16 out(pix.RAW); out /= in;
		return out;
	}
};