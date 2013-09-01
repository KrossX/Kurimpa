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

s8 DitherMatrix[4][4] =
{
	{-4,  0, -3, +1}, // First two scanlines
	{+2, -2, +3, -1}, 
	{-3, +1, -4,  0}, // other two scanlines
	{+3, -1, +2, -2}
};

static inline u32 ModulateTex(u32 color, u16 texel)
{
	RGBA5551 t; t.RAW = texel;
	RGBA8 c; c.RAW = color;

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
	RGBA8 pin; pin.RAW = in;

	RGBA5551 pix16;
	pix16.R = pin.R >> 3;
	pix16.G = pin.G >> 3;
	pix16.B = pin.B >> 3;
	pix16.A = 0;

	return pix16.RAW;
}

static inline u32 GetPix24(u16 in)
{
	RGBA5551 pin; pin.RAW = in;

	RGBA8 pix32;
	pix32.R = (pin.R * 0xFF) / 0x1F;
	pix32.G = (pin.G * 0xFF) / 0x1F;
	pix32.B = (pin.B * 0xFF) / 0x1F;
	pix32.A = 0;

	return pix32.RAW;
}

static inline u16 GetPix16Dither(u32 in, s16 posx, s16 posy)
{
	RGBA8 pin; pin.RAW = in;

	s8 offset = DitherMatrix[posx % 4][posy % 4];

	s16 R = pin.R + offset;
	s16 G = pin.G + offset;
	s16 B = pin.B + offset;

	pin.R = (R > 0xFF ? 0xFF : R < 0 ? 0 : R) & 0xFF;
	pin.G = (G > 0xFF ? 0xFF : G < 0 ? 0 : G) & 0xFF;
	pin.B = (B > 0xFF ? 0xFF : B < 0 ? 0 : B) & 0xFF;

	return GetPix16(pin.RAW);
}

static inline VEC3 GetColorDiff(u32 c1, u32 c2, int len)
{
	if(!len) return VEC3();

	VEC3 out;
	RGBA8 C1, C2;
	C1.RAW = c1;
	C2.RAW = c2;

	out.R = (C2.R - C1.R) / (float)len;
	out.G = (C2.G - C1.G) / (float)len;
	out.B = (C2.B - C1.B) / (float)len;

	return out;
}

static inline u32 GetColorGrad(u32 c0, VEC3 diff, int i)
{
	RGBA8 C0; C0.RAW = c0;

	C0.R += (u8)(diff.R * i);
	C0.G += (u8)(diff.G * i);
	C0.B += (u8)(diff.B * i);

	return C0.RAW;
}

static inline u32 GetColorBlend3(vectk &v0, vectk &v1, vectk &v2, float s, float t)
{
	RGBA8 C0; C0.RAW = v0.c;
	RGBA8 C1; C1.RAW = v1.c;
	RGBA8 C2; C2.RAW = v2.c;

	C0.R = (u8)(C0.R * (1.0f - (s+t)) + C1.R * s + C2.R * t);
	C0.G = (u8)(C0.G * (1.0f - (s+t)) + C1.G * s + C2.G * t);
	C0.B = (u8)(C0.B * (1.0f - (s+t)) + C1.B * s + C2.B * t);

	return C0.RAW;
}

static inline u16 GetTexCoord3(vectk &v0, vectk &v1, vectk &v2, float s, float t)
{
	Half out;

	float st = 1.0f - s - t;
	float u = v0.u * st + v1.u * s + v2.u * t + 0.5f;
	float v = v0.v * st + v1.v * s + v2.v * t + 0.5f;

	out.U8[0] = (u8)u;
	out.U8[1] = (u8)v;

	return out.U16;
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

inline u16 PSXgpu::Blend(u16 &back, u16 &front)
{
	s16 Br, Bg, Bb;
	s16 Fr, Fg, Fb;

	Br = back & 0x1F;
	Bg = (back >> 5) & 0x1F;
	Bb = (back >> 10) & 0x1F;

	Fr = front & 0x1F;
	Fg = (front >> 5) & 0x1F;
	Fb = (front >> 10) & 0x1F;

	u8 mode = ((GPUSTAT & GPUSTAT_BLENDEQ) >> 5) & 3;

	switch(mode)
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

void PSXgpu::SetPixel(u32 color, s16 x, s16 y, u16 texel = 0)
{
	DA.ApplyOffset(x, y);
	if(DA.ScissorTest(x, y)) return;

	u16 frontcolor, backcolor = VRAM.HALF2[y][x];
	if(doMaskCheck(backcolor)) return;

	bool texMasked = !!(texel & 0x8000);

	bool isTextured = !!(DC & DCMD_TEXTURED);
	bool isRect = (DC & DCMD_TYPE) == 0x60; // 0x20 poly, 0x40 line, 0x60 rect
	bool isGouraud = !!(DC & DCMD_GOURAUD);
	bool isMaskForced = !!(GPUSTAT & GPUSTAT_MASKSET);

	bool doModulate = !(DC & DCMD_ENVREPLACE);
	bool doBlend = !!(DC & DCMD_BLENDENABLED) && (!isTextured || (isTextured && texMasked));
	bool doDither = !!(GPUSTAT & GPUSTAT_DITHER) && (!isRect && isGouraud);
	bool doMask = isMaskForced || texMasked;
	
	if(isTextured)
		color = doModulate? ModulateTex(color, texel) : GetPix24(texel);

	frontcolor = doDither? GetPix16Dither(color, x, y) : GetPix16(color);
	
	if(doBlend)
		frontcolor = Blend(backcolor, frontcolor);

	if(!DI.is480i) SetDrawLine(y%2);

	VRAM.HALF2[y][x] = doMask ? frontcolor | 0x8000 : frontcolor;
}

u16 PSXgpu::GetTexel(u8 tx, u8 ty)
{
	Half color; color.U16 = 0;

	TW.ApplyMaskOffset(tx, ty);

	u8 colormode = ((GPUSTAT & GPUSTAT_PAGECOL) >> 7) & 3;

	switch(colormode)
	{
	case 0: // 4 bits
		color.U16 = TW.PAGE[(ty << 10) + (tx >> 2)];

		switch(tx % 4)
		{
		case 0: color.U16 = TW.CLUT[color.U8[0] & 0xF]; break;
		case 1: color.U16 = TW.CLUT[(color.U8[0] >> 4) & 0xF]; break;
		case 2: color.U16 = TW.CLUT[color.U8[1] & 0xF]; break;
		case 3: color.U16 = TW.CLUT[(color.U8[1] >> 4) & 0xF]; break;
		}
		break;

	case 1: // 8 bits
		color.U16 = TW.PAGE[(ty << 10) + (tx >> 1)];
		color.U16 = TW.CLUT[tx % 2 ? color.U8[1] : color.U8[0]];
		break;

	case 2: // 15 bits
	case 3: // Reserved... also 15 bits?
		color.U16 = TW.PAGE[(ty << 10) + tx];
		break;
	}

	return color.U16;
}

template <RENDERTYPE render_mode>
void PSXgpu::RasterLine()
{
	//  THE EXTREMELY FAST LINE ALGORITHM (B)
	// http://www.edepot.com/algorithm.html

	bool yLonger=false;
	int incrementVal;
	int shortLen = vertex[1].y - vertex[0].y;
	int longLen = vertex[1].x - vertex[0].x;

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

template <RENDERTYPE render_mode>
void PSXgpu::RasterPoly3()
{
	vectk vs1, vs2, temp;
	vs1.x = vertex[1].x - vertex[0].x;
	vs1.y = vertex[1].y - vertex[0].y;
	vs2.x = vertex[2].x - vertex[0].x;
	vs2.y = vertex[2].y - vertex[0].y;

	s16 minX = min(min(vertex[0].x, vertex[1].x), vertex[2].x);
	s16 maxX = max(max(vertex[0].x, vertex[1].x), vertex[2].x);
	s16 minY = min(min(vertex[0].y, vertex[1].y), vertex[2].y);
	s16 maxY = max(max(vertex[0].y, vertex[1].y), vertex[2].y);

	if((maxX - minX) > 1023) return;
	if((maxY - minY) > 511) return;

	DA.PolyAreaClip(minX, maxX, minY, maxY);

	float cross = crossProduct(vs2, vs1);
	if(!cross) return;

	Half tx;
	u16 texel;
	s16 x, y;

	float s, t;

	switch(render_mode)
	{
	case RENDER_FLAT:
		for (y = minY; y < maxY; y++)
		{
			temp.y = y - vertex[0].y;

			for (x = minX; x < maxX; x++)
			{
				temp.x = x - vertex[0].x;

				s = crossProduct(temp, vs2) / -cross;
				if((s < 0) || (s > 1)) continue;

				t = crossProduct(temp, vs1) /  cross;

				if((t >= 0) && ((s + t) <= 1))
				{
					SetPixel(vertex[0].c, x, y);
				}
			}
		}
		break;

	case RENDER_TEXTURED:
		for (y = minY; y < maxY; y++)
		{
			temp.y = y - vertex[0].y;

			for (x = minX; x < maxX; x++)
			{
				temp.x = x - vertex[0].x;

				s = crossProduct(temp, vs2) / -cross;
				if((s < 0) || (s > 1)) continue;

				t = crossProduct(temp, vs1) /  cross;

				if((t >= 0) && ((s + t) <= 1))
				{ 
					tx.U16 = GetTexCoord3(vertex[0], vertex[1], vertex[2], s, t);
					texel = GetTexel(tx.U8[0], tx.U8[1]);

					if(texel)
						SetPixel(vertex[0].c, x, y, texel);
				}
			}
		}
		break;

	case RENDER_SHADED:
		for (y = minY; y < maxY; y++)
		{
			temp.y = y - vertex[0].y;

			for (x = minX; x < maxX; x++)
			{
				temp.x = x - vertex[0].x;
				
				s = crossProduct(temp, vs2) / -cross;
				if((s < 0) || (s > 1)) continue;

				t = crossProduct(temp, vs1) /  cross;

				if((t >= 0) && ((s + t) <= 1))
				{ 
					SetPixel(GetColorBlend3(vertex[0], vertex[1], vertex[2], s, t), x, y);
				}
			}
		}
		break;

	case RENDER_SHADED_TEXTURED:
		for (y = minY; y < maxY; y++)
		{
			temp.y = y - vertex[0].y;

			for (x = minX; x < maxX; x++)
			{
				temp.x = x - vertex[0].x;

				s = crossProduct(temp, vs2) / -cross;
				if((s < 0) || (s > 1)) continue;

				t = crossProduct(temp, vs1) /  cross;

				if((t >= 0) && ((s + t) <= 1))
				{ 
					tx.U16 = GetTexCoord3(vertex[0], vertex[1], vertex[2], s, t);
					texel = GetTexel(tx.U8[0], tx.U8[1]);

					u32 color = (DC & DCMD_ENVREPLACE) ? 0 : GetColorBlend3(vertex[0], vertex[1], vertex[2], s, t);

					if(texel)
						SetPixel(color, x, y, texel);
				}
			}
		}
		break;
	}
}

template <RENDERTYPE render_mode>
void PSXgpu::RasterPoly4()
{
	RasterPoly3<render_mode>();

	if((render_mode == RENDER_FLAT) || (render_mode == RENDER_TEXTURED))
		vertex[1].c = vertex[0].c;

	vertex[0] = vertex[1];
	vertex[1] = vertex[2];
	vertex[2] = vertex[3];

	RasterPoly3<render_mode>();
}

inline void PSXgpu::DrawStart(u32 data)
{
	SetReadyCMD(0);
	SetReadyDMA(0);
	vertex[0].c = data & 0xFFFFFF;
	DC = (data >> 24) & 0xFF;
}

void PSXgpu::DrawPoly3(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].SetV(data); break;
	case 3: vertex[2].SetV(data);

		RasterPoly3<RENDER_FLAT>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly3T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3T;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].SetV(data); break;
	case 4: vertex[1].SetT(data);
		SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 5: vertex[2].SetV(data); break;
	case 6: vertex[2].SetT(data);

		RasterPoly3<RENDER_TEXTURED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly3S(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3S;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;
	case 3: vertex[1].SetV(data); break;
	case 4: vertex[2].c = data; break;
	case 5: vertex[2].SetV(data);

		RasterPoly3<RENDER_SHADED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly3ST(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3ST;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].c = data; break;
	case 4: vertex[1].SetV(data); break;
	case 5: vertex[1].SetT(data);
		SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 6: vertex[2].c = data; break;
	case 7: vertex[2].SetV(data); break;
	case 8: vertex[2].SetT(data);

		RasterPoly3<RENDER_SHADED_TEXTURED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].SetV(data); break;
	case 3: vertex[2].SetV(data); break;
	case 4: vertex[3].SetV(data);

		RasterPoly4<RENDER_FLAT>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4T;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].SetV(data); break;
	case 4: vertex[1].SetT(data);
		SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 5: vertex[2].SetV(data); break;
	case 6: vertex[2].SetT(data); break;
	case 7: vertex[3].SetV(data); break;
	case 8: vertex[3].SetT(data);

		RasterPoly4<RENDER_TEXTURED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4S(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4S;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;
	case 3: vertex[1].SetV(data); break;
	case 4: vertex[2].c = data; break;
	case 5: vertex[2].SetV(data); break;
	case 6: vertex[3].c = data; break;
	case 7: vertex[3].SetV(data);

		RasterPoly4<RENDER_SHADED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4ST(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4ST;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].c = data; break;
	case 4: vertex[1].SetV(data); break;
	case 5: vertex[1].SetT(data);
		SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 6: vertex[2].c = data; break;
	case 7: vertex[2].SetV(data); break;
	case 8: vertex[2].SetT(data); break;
	case 9: vertex[3].c = data; break;
	case 10:vertex[3].SetV(data); break;
	case 11:vertex[3].SetT(data);

		RasterPoly4<RENDER_SHADED_TEXTURED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawLine(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINE;
		break;

	case 1: vertex[0].SetV(data);
		break;

	case 2:
		vertex[1].SetV(data);

		RasterLine<RENDER_FLAT>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawLineS(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINES;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: 
		vertex[1].SetV(data);
		RasterLine<RENDER_FLAT>();
		break;

	default:
		if(data == 0x55555555) SetReady();
		else
		{
			u8 vx = (gpcount % 2) ? 0 : 1; 
			vertex[vx].SetV(data);
			RasterLine<RENDER_FLAT>();
		}
		break;
	}
}

void PSXgpu::DrawLineG(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINEG;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;
	case 3:
		vertex[1].SetV(data);

		RasterLine<RENDER_SHADED>();
		SetReady();
		break;
	}
}

void PSXgpu::DrawLineGS(u32 data)
{
	static u32 line = 0;

	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINEGS;
		line = 0;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;

	case 3:
		vertex[1].SetV(data);
		RasterLine<RENDER_SHADED>();
		break;

	default:
		if(data == 0x55555555) SetReady();
		else
		{
			if(line % 2)
			{
				vertex[2].SetV(data);

				vertex[0] = vertex[1];
				vertex[1] = vertex[2];

				RasterLine<RENDER_SHADED>();
			}
			else
			{
				vertex[2].c = data;
			}

			line++;
		}
		break;
	}
}

void PSXgpu::DrawRect0(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT0;
		break;

	case 1:
		vertex[0].SetV(data);
		break;

	case 2:
		{
			vertex[1].SetV(data); // Size
			s16 x, y, xmin, xmax, ymin, ymax;

			xmin = vertex[0].x; xmax = xmin + vertex[1].x;
			ymin = vertex[0].y; ymax = ymin + vertex[1].y;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel(vertex[0].c, x, y);
		}
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect0T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT0T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3:
		{
			vertex[1].SetV(data); // Size
			s16 x, y, xmin, xmax, ymin, ymax;
			u8 tx, ty;

			xmin = vertex[0].x; xmax = xmin + vertex[1].x;
			ymin = vertex[0].y; ymax = ymin + vertex[1].y;

			for(y = ymin, ty = vertex[0].v; y < ymax; y++, ty++)
			for(x = xmin, tx = vertex[0].u; x < xmax; x++, tx++)
			{
				u16 texel = GetTexel(tx, ty);

				if(texel)
					SetPixel(vertex[0].c, x, y, texel);
			}
		}
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect1(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT1;
		break;

	case 1:
		{
			vertex[0].SetV(data);

			SetPixel(vertex[0].c, vertex[0].x, vertex[0].y);
		}
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect1T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT1T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		{
			vertex[0].SetT(data);
			TW.SetCLUT(VRAM.HALF2, vertex[0].tex);

			u16 texel = GetTexel(vertex[0].u, vertex[0].v);

			if(texel)
				SetPixel(vertex[0].c, vertex[0].x, vertex[0].y, texel);
		}
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect8(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT8;
		break;

	case 1:
		{
			vertex[0].SetV(data);
			s16 x, y, xmin, xmax, ymin, ymax;

			xmin = vertex[0].x; xmax = xmin + 8;
			ymin = vertex[0].y; ymax = ymin + 8;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel(vertex[0].c, x, y);
		}
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect8T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT8T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		{
			vertex[0].SetT(data);
			TW.SetCLUT(VRAM.HALF2, vertex[0].tex);

			s16 x, y, xmin, xmax, ymin, ymax;
			u8 tx, ty;

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
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect16(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT16;
		break;

	case 1:
		{
			vertex[0].SetV(data);
			s16 x, y, xmin, xmax, ymin, ymax;

			xmin = vertex[0].x; xmax = xmin + 16;
			ymin = vertex[0].y; ymax = ymin + 16;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel(vertex[0].c, x, y);
		}
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect16T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT16T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		{
			vertex[0].SetT(data);
			TW.SetCLUT(VRAM.HALF2, vertex[0].tex);

			s16 x, y, xmin, xmax, ymin, ymax;
			u8 tx, ty;

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
		SetReady();
		break;
	}
}


void PSXgpu::DrawFill(u32 data)
{
	static u16 color;

	switch(gpcount)
	{
	case 0:
		color = GetPix16(data);
		SetReadyCMD(0);
		gpsize = 2;
		GPUMODE = GPUMODE_FILL;
		break;

	case 1:
		TR.start.U32 = data;
		TR.start.U16[0] &= 0x3F0;
		TR.start.U16[1] &= 0x1FF;
		break;

	case 2:
		{
			TR.size.U32 = data;
			if(!TR.size.U16[0] || !TR.size.U16[1]) return;

			TR.size.U16[0]  = ((TR.size.U16[0] & 0x3FF) + 0x0F) & ~0x0F;
			TR.size.U16[1] &= 0x1FF;

			const u16 xmax =  TR.start.U16[0] + TR.size.U16[0];
			const u16 ymax =  TR.start.U16[1] + TR.size.U16[1];

			const u16 xmin = TR.start.U16[0];
			const u16 ymin = TR.start.U16[1];


			for(u16 y = ymin; y < ymax; y++)
			for(u16 x = xmin; x < xmax; x++)
				VRAM.HALF2[y][x] = color;

			SetReady();
		}
		break;
	}
}