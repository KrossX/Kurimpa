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

#include "General.h"
#include "PSXgpu_Enums.h"
#include "PSXgpu.h"
#include "RasterPSX.h"
#include <cmath>


s8 DitherMatrix[4][4] =
{
	{-4,  0, -3, +1}, // First two scanlines
	{+2, -2, +3, -1}, 
	{-3, +1, -4,  0}, // other two scanlines
	{+3, -1, +2, -2}
};

static inline u32 ModulateTex(u32 color, u16 texel)
{
	RGBA5551 t(texel);
	RGBA8 c(color);

	u16 Cr = (c.R * t.R) >> 4;
	u16 Cg = (c.G * t.G) >> 4;
	u16 Cb = (c.B * t.B) >> 4;

	c.R = Cr > 0xFF ? 0xFF : Cr;
	c.G = Cg > 0xFF ? 0xFF : Cg;
	c.B = Cb > 0xFF ? 0xFF : Cb;

	return c.RAW;
}

static inline u16 GetPix16(u32 in)
{
	RGBA8 pin(in);

	RGBA5551 pix16(0);
	pix16.R = pin.R >> 3;
	pix16.G = pin.G >> 3;
	pix16.B = pin.B >> 3;

	return pix16.RAW;
}

static inline u32 GetPix24(u16 in)
{
	RGBA5551 pin(in);

	RGBA8 pix32(0);
	pix32.R = (pin.R * 0xFF) / 0x1F;
	pix32.G = (pin.G * 0xFF) / 0x1F;
	pix32.B = (pin.B * 0xFF) / 0x1F;

	return pix32.RAW;
}

static inline u16 GetPix16Dither(u32 in, s16 posx, s16 posy)
{
	RGBA8 pin(in);

	s8 offset = DitherMatrix[posx % 4][posy % 4];

	s16 R = pin.R + offset;
	s16 G = pin.G + offset;
	s16 B = pin.B + offset;

	pin.R = R > 0xFF ? 0xFF : R < 0 ? 0 : R;
	pin.G = G > 0xFF ? 0xFF : G < 0 ? 0 : G;
	pin.B = B > 0xFF ? 0xFF : B < 0 ? 0 : B;

	return GetPix16(pin.RAW);
}

static inline VEC3 GetColorDiff(u32 c1, u32 c2, int len)
{
	if(!len) return VEC3();

	VEC3 out;
	RGBA8 C1(c1), C2(c2);

	out.R = (C2.R - C1.R) / (float)len;
	out.G = (C2.G - C1.G) / (float)len;
	out.B = (C2.B - C1.B) / (float)len;

	return out;
}

static inline u32 GetColorGrad(u32 c0, VEC3 diff, int i)
{
	RGBA8 C0(c0);

	C0.R += (u8)(diff.R * i);
	C0.G += (u8)(diff.G * i);
	C0.B += (u8)(diff.B * i);

	return C0.RAW;
}

static inline u32 GetColorBlend3(vectk *v, float s, float t)
{
	RGBA8 C0(v[0].c), C1(v[1].c), C2(v[2].c);

	C0.R = (u8)(C0.R * (1.0f - (s+t)) + C1.R * s + C2.R * t);
	C0.G = (u8)(C0.G * (1.0f - (s+t)) + C1.G * s + C2.G * t);
	C0.B = (u8)(C0.B * (1.0f - (s+t)) + C1.B * s + C2.B * t);

	return C0.RAW;
}

static inline void ClampMax(s16 &r, s16 &g, s16 &b, s16 max)
{
	r = r > 0x1F ? 0x1F : r;
	g = g > 0x1F ? 0x1F : g;
	b = b > 0x1F ? 0x1F : b;
}

static inline void ClampMin(s16 &r, s16 &g, s16 &b, s16 max)
{
	r = r < 0 ? 0 : r;
	g = g < 0 ? 0 : g;
	b = b < 0 ? 0 : b;
}

inline u16 RasterPSXSW::Blend(u16 &back, u16 &front)
{
	s16 Br, Bg, Bb;
	s16 Fr, Fg, Fb;

	Br = back & 0x1F;
	Bg = (back >> 5) & 0x1F;
	Bb = (back >> 10) & 0x1F;

	Fr = front & 0x1F;
	Fg = (front >> 5) & 0x1F;
	Fb = (front >> 10) & 0x1F;

	switch(GPUSTAT->BLENDEQ)
	{
	case 0:
		Fr = (Br + Fr) >> 1;
		Fg = (Bg + Fg) >> 1;
		Fb = (Bb + Fb) >> 1;
		break;
		
	case 1:
		Fr = Br + Fr;
		Fg = Bg + Fg;
		Fb = Bb + Fb;
		ClampMax(Fr, Fg, Fb, 0x1F);
		break;
		
	case 2:
		Fr = Br - Fr;
		Fg = Bg - Fg;
		Fb = Bb - Fb;
		ClampMin(Fr, Fg, Fb, 0x00);
		break;

	case 3:
		Fr = Br + (Fr >> 2);
		Fg = Bg + (Fg >> 2);
		Fb = Bb + (Fb >> 2);
		ClampMax(Fr, Fg, Fb, 0x1F);
		break;
	}

	u16 out = Fr | (Fg << 5) | (Fb << 10);
	return out;
}

void RasterPSXSW::SetPixel(u32 color, s16 x, s16 y, u16 texel = 0)
{
	DA->ApplyOffset(x, y);
	if(DA->ScissorTest(x, y)) return;

	u16 frontcolor, backcolor = VRAM->HALF2[y][x];
	if(doMaskCheck(backcolor)) return;

	bool texMasked = !!(texel & 0x8000);
	bool doBlend = Drawing.isBlendEnabled && (!Drawing.isTextured || (Drawing.isTextured && texMasked));
	bool doMask = Drawing.isMaskForced || texMasked;
	
	if(Drawing.isTextured)
		color = Drawing.doModulate? ModulateTex(color, texel) : GetPix24(texel);

	frontcolor = Drawing.doDither? GetPix16Dither(color, x, y) : GetPix16(color);
	
	if(doBlend)
		frontcolor = Blend(backcolor, frontcolor);

	VRAM->HALF2[y][x] = doMask ? frontcolor | 0x8000 : frontcolor;
}

u16 RasterPSXSW::GetTexel(u8 tx, u8 ty)
{
	Half color; color.U16 = 0;

	TW->ApplyMaskOffset(tx, ty);

	switch(GPUSTAT->PAGECOL)
	{
	case 0: // 4 bits
		color.U16 = TW->PAGE[(ty << 10) + (tx >> 2)];

		switch(tx % 4)
		{
		case 0: color.U16 = TW->CLUT[color.U8[0] & 0xF]; break;
		case 1: color.U16 = TW->CLUT[(color.U8[0] >> 4) & 0xF]; break;
		case 2: color.U16 = TW->CLUT[color.U8[1] & 0xF]; break;
		case 3: color.U16 = TW->CLUT[(color.U8[1] >> 4) & 0xF]; break;
		}
		break;

	case 1: // 8 bits
		color.U16 = TW->PAGE[(ty << 10) + (tx >> 1)];
		color.U16 = TW->CLUT[tx % 2 ? color.U8[1] : color.U8[0]];
		break;

	case 2: // 15 bits
	case 3: // Reserved... also 15 bits?
		color.U16 = TW->PAGE[(ty << 10) + tx];
		break;
	}

	return color.U16;
}

u16 RasterPSXSW::GetTexel3(vectk *v, float s, float t)
{
	float st = 1.0f - s - t;
	float U = v[0].u * st + v[1].u * s + v[2].u * t + 0.5f;
	float V = v[0].v * st + v[1].v * s + v[2].v * t + 0.5f;
	
	return GetTexel((u8)U, (u8)V);
}

void RasterPSXSW::RasterLine(RENDERTYPE render_mode)
{
	//  THE EXTREMELY FAST LINE ALGORITHM (B)
	// http://www.edepot.com/algorithm.html

	bool yLonger=false;
	int incrementVal;
	int shortLen = vertex[1].y - vertex[0].y;
	int longLen = vertex[1].x - vertex[0].x;

	if(longLen > 1023 || shortLen > 511) return;

	if(!shortLen) shortLen = 1;
	if(!longLen) longLen = 1;

	if (abs(shortLen)>abs(longLen))
	{
		int swap=shortLen;
		shortLen=longLen;
		longLen=swap;
		yLonger=true;
	}

	incrementVal = longLen < 0 ? -1 : 1;
	float multDiff = (!longLen)? (float)shortLen : (float)shortLen/(float)longLen;

	if(render_mode == RENDER_SHADED)
	{
		VEC3 cdiff = GetColorDiff(vertex[0].c, vertex[1].c, longLen);

		if (yLonger) for (int i=0; i!=longLen; i+=incrementVal)
		{
			SetPixel(GetColorGrad(vertex[0].c, cdiff, i), vertex[0].x+(int)((float)i*multDiff), vertex[0].y+i);
		}
		else for (int i=0; i!=longLen; i+=incrementVal)
		{
			SetPixel(GetColorGrad(vertex[0].c, cdiff, i),  vertex[0].x+i, vertex[0].y+(int)((float)i*multDiff));
		}
	}
	else
	{
		if (yLonger) for (int i=0; i!=longLen; i+=incrementVal)
		{
			SetPixel(vertex[0].c, vertex[0].x+(int)((float)i*multDiff), vertex[0].y+i);
		}
		else for (int i=0; i!=longLen; i+=incrementVal)
		{
			SetPixel(vertex[0].c, vertex[0].x+i, vertex[0].y+(int)((float)i*multDiff));
		}
	}
}

float crossProduct(vectk &a, vectk &b)
{
	return (float)((a.x * b.y) - (a.y * b.x));
}

void RasterPSXSW::RasterPoly3(RENDERTYPE render_mode)
{
	s16 minX = min(min(vertex[0].x, vertex[1].x), vertex[2].x);
	s16 maxX = max(max(vertex[0].x, vertex[1].x), vertex[2].x);
	s16 minY = min(min(vertex[0].y, vertex[1].y), vertex[2].y);
	s16 maxY = max(max(vertex[0].y, vertex[1].y), vertex[2].y);

	if((maxX - minX) > 1023) return;
	if((maxY - minY) > 511) return;

	DA->PolyAreaClip(minX, maxX, minY, maxY);

	if(minX == maxX) maxX++;
	if(minY == maxY) maxY++;

	vectk vs1, vs2, temp;
	vs1.x = vertex[1].x - vertex[0].x;
	vs1.y = vertex[1].y - vertex[0].y;
	vs2.x = vertex[2].x - vertex[0].x;
	vs2.y = vertex[2].y - vertex[0].y;

	float cross = crossProduct(vs2, vs1);
	if(!cross) return;

	u16 texel;
	s16 x, y;

	float s, t;

	switch(render_mode)
	{
	case RENDER_FLAT:
		for (y = minY; y < maxY; y++)
		for (x = minX; x < maxX; x++)
		{
			temp.y = y - vertex[0].y;
			temp.x = x - vertex[0].x;

			s = crossProduct(temp, vs2) / -cross;
			t = crossProduct(temp, vs1) /  cross;

			if((s >= 0) && (t >= 0) && ((s + t) <= 1))
			{
				SetPixel(vertex[0].c, x, y);
			}
		}
		break;

	case RENDER_TEXTURED:
		for (y = minY; y < maxY; y++)
		for (x = minX; x < maxX; x++)
		{
			temp.y = y - vertex[0].y;
			temp.x = x - vertex[0].x;

			s = crossProduct(temp, vs2) / -cross;
			t = crossProduct(temp, vs1) /  cross;

			if((s >= 0) && (t >= 0) && ((s + t) <= 1))
			{ 
				texel = GetTexel3(vertex, s, t);

				if(texel)
					SetPixel(vertex[0].c, x, y, texel);
			}
		}
		break;

	case RENDER_SHADED:
		for (y = minY; y < maxY; y++)
		for (x = minX; x < maxX; x++)
		{
			temp.y = y - vertex[0].y;
			temp.x = x - vertex[0].x;
				
			s = crossProduct(temp, vs2) / -cross;
			t = crossProduct(temp, vs1) /  cross;

			if((s >= 0) && (t >= 0) && ((s + t) <= 1))
			{ 
				SetPixel(GetColorBlend3(vertex, s, t), x, y);
			}
		}
		break;

	case RENDER_SHADED_TEXTURED:
		for (y = minY; y < maxY; y++)
		for (x = minX; x < maxX; x++)
		{
			temp.y = y - vertex[0].y;
			temp.x = x - vertex[0].x;

			s = crossProduct(temp, vs2) / -cross;
			t = crossProduct(temp, vs1) /  cross;

			if((s >= 0) && (t >= 0) && ((s + t) <= 1))
			{ 
				texel = GetTexel3(vertex, s, t);
				
				if(texel)
					SetPixel(GetColorBlend3(vertex, s, t), x, y, texel);
			}
		}
		break;
	}
}

void RasterPSXSW::RasterPoly4(RENDERTYPE render_mode)
{
	RasterPoly3(render_mode);

	if((render_mode == RENDER_FLAT) || (render_mode == RENDER_TEXTURED))
		vertex[1].c = vertex[0].c;

	vertex[0] = vertex[1];
	vertex[1] = vertex[2];
	vertex[2] = vertex[3];

	RasterPoly3(render_mode);
}

void RasterPSXSW::RasterRect(RENDERTYPE render_mode, u8 type)
{
	s16 x, y, xmin, xmax, ymin, ymax;
	u8 tx, ty;

	switch(type)
	{
	case 0: // WxH
		xmin = vertex[0].x; xmax = xmin + vertex[1].x;
		ymin = vertex[0].y; ymax = ymin + vertex[1].y;

		if(xmin == xmax) xmax++; 
		if(ymin == ymax) ymax++;

		if(render_mode == RENDER_TEXTURED)
		{
			for(y = ymin, ty = vertex[0].v; y < ymax; y++, ty++)
			for(x = xmin, tx = vertex[0].u; x < xmax; x++, tx++)
			{
				u16 texel = GetTexel(tx, ty);

				if(texel)
					SetPixel(vertex[0].c, x, y, texel);
			}
		}
		else
		{
			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel(vertex[0].c, x, y);
		}
		break;

	case 1: // 1x1
		if(render_mode == RENDER_TEXTURED)
		{
			u16 texel = GetTexel(vertex[0].u, vertex[0].v);

			if(texel)
				SetPixel(vertex[0].c, vertex[0].x, vertex[0].y, texel);
		}
		else
		{
			SetPixel(vertex[0].c, vertex[0].x, vertex[0].y);
		}
		break;

	case 2: // 8x8
		if(render_mode == RENDER_TEXTURED)
		{
			xmin = vertex[0].x; xmax = xmin + 8;
			ymin = vertex[0].y; ymax = ymin + 8;

			for(y = ymin, ty = vertex[0].v; y < ymax; y++, ty++)
			for(x = xmin, tx = vertex[0].u; x < xmax; x++, tx++)
			{
				u16 texel = GetTexel(tx, ty);

				if(texel)
					SetPixel(vertex[0].c, x, y, texel);
			}
		}
		else
		{
			xmin = vertex[0].x; xmax = xmin + 8;
			ymin = vertex[0].y; ymax = ymin + 8;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel(vertex[0].c, x, y);
		}
		break;

	case 3: // 16x16
		if(render_mode == RENDER_TEXTURED)
		{
			xmin = vertex[0].x; xmax = xmin + 16;
			ymin = vertex[0].y; ymax = ymin + 16;

			for(y = ymin, ty = vertex[0].v; y < ymax; y++, ty++)
			for(x = xmin, tx = vertex[0].u; x < xmax; x++, tx++)
			{
				u16 texel = GetTexel(tx, ty);

				if(texel)
					SetPixel(vertex[0].c, x, y, texel);
			}
		}
		else
		{
			xmin = vertex[0].x; xmax = xmin + 16;
			ymin = vertex[0].y; ymax = ymin + 16;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel(vertex[0].c, x, y);
		}
		break;
	}
}


void RasterPSXSW::DrawFill()
{
	u16 color16 = GetPix16(vertex[0].c);

	const u16 xmax =  TR->start.U16[0] + TR->size.U16[0];
	const u16 ymax =  TR->start.U16[1] + TR->size.U16[1];

	const u16 xmin = TR->start.U16[0];
	const u16 ymin = TR->start.U16[1];

	for(u16 y = ymin; y < ymax; y++)
	for(u16 x = xmin; x < xmax; x++)
		VRAM->HALF2[y][x] = color16;
}
