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

const s8 DitherMatrix[4][4] =
{
	{-4,  0, -3, +1},
	{+2, -2, +3, -1},
	{-3, +1, -4,  0},
	{+3, -1, +2, -2}
};

static inline u32 GetColorBlend3(vectk *v, float s, float t)
{
	RGBA8 C0(v[0].c), C1(v[1].c), C2(v[2].c);

	const float st = 1.0f - (s+t);

	C0.R = (u8)(C0.R * st + C1.R * s + C2.R * t);
	C0.G = (u8)(C0.G * st + C1.G * s + C2.G * t);
	C0.B = (u8)(C0.B * st + C1.B * s + C2.B * t);

	return C0.RAW;
}

template<bool textured>
void RasterPSXSW::SetPixel(u32 color, s16 x, s16 y, u16 texel)
{
	if(DA->ScissorTest(x, y)) return;

	u16 &backcolor = VRAM->HALF2[y][x];

	if(doMaskCheck(backcolor)) return;

	const u16 texmask = texel & 0x8000;

	RGBA5551 tex(texel), front(0);
	RGBA8 col(color);
	
	if(textured)
	{
		if(Drawing.doModulate)
		{
			u16 Cr = (col.R * tex.R) >> 4;
			u16 Cg = (col.G * tex.G) >> 4;
			u16 Cb = (col.B * tex.B) >> 4;

			col.R = Cr > 0xFF ? 0xFF : Cr;
			col.G = Cg > 0xFF ? 0xFF : Cg;
			col.B = Cb > 0xFF ? 0xFF : Cb;
		}
		else 
		{
			col.R = (tex.R * 0xFF) / 0x1F;
			col.G = (tex.G * 0xFF) / 0x1F;
			col.B = (tex.B * 0xFF) / 0x1F;
		}
	}

	if(Drawing.doDither)
	{
		s8 offset = DitherMatrix[y&3][x&3];

		s16 R = col.R + offset;
		s16 G = col.G + offset;
		s16 B = col.B + offset;

		col.R = R > 0xFF ? 0xFF : R < 0 ? 0 : R;
		col.G = G > 0xFF ? 0xFF : G < 0 ? 0 : G;
		col.B = B > 0xFF ? 0xFF : B < 0 ? 0 : B;
	}

	front.R = col.R >> 3;
	front.G = col.G >> 3;
	front.B = col.B >> 3;
	
	if(Drawing.isBlendEnabled)
	{
		if(!textured || (texmask > 0))
		{
			RGBA5551 back(backcolor);

			s8 R, G, B;

			switch(GPUSTAT->BLENDEQ)
			{
			case 0:
				front.R = (back.R + front.R) >> 1;
				front.G = (back.G + front.G) >> 1;
				front.B = (back.B + front.B) >> 1;
				break;
		
			case 1:
				R = back.R + front.R;
				G = back.G + front.G;
				B = back.B + front.B;

				front.R = R > 0x1F ? 0x1F : R;
				front.G = G > 0x1F ? 0x1F : G;
				front.B = B > 0x1F ? 0x1F : B;
				break;
		
			case 2:
				R = back.R - front.R;
				G = back.G - front.G;
				B = back.B - front.B;

				front.R = R < 0 ? 0 : R;
				front.G = G < 0 ? 0 : G;
				front.B = B < 0 ? 0 : B;
				break;

			case 3:
				R = back.R + (front.R >> 2);
				G = back.G + (front.G >> 2);
				B = back.B + (front.B >> 2);

				front.R = R > 0x1F ? 0x1F : R;
				front.G = G > 0x1F ? 0x1F : G;
				front.B = B > 0x1F ? 0x1F : B;
				break;
			}
		}
	}

	backcolor = front.RAW | GPUSTAT->MASK16 | texmask;
}

u16 RasterPSXSW::GetTexel(u8 tx, u8 ty)
{
	Half color; color.U16 = 0;

	TW->ApplyMaskOffset(tx, ty);

	switch(GPUSTAT->PAGECOL)
	{
	case 0: // 4 bits
		color.U16 = TW->PAGE[(ty << 10) + (tx >> 2)];

		switch(tx & 3)
		{
		case 0: return TW->CLUT[color.U8[0] & 0xF];
		case 1: return TW->CLUT[(color.U8[0] >> 4) & 0xF];
		case 2: return TW->CLUT[color.U8[1] & 0xF];
		case 3: return TW->CLUT[(color.U8[1] >> 4) & 0xF];
		}
		break;

	case 1: // 8 bits
		color.U16 = TW->PAGE[(ty << 10) + (tx >> 1)];
		return TW->CLUT[color.U8[tx&1]];

	case 2: // 15 bits
	case 3: // Reserved... also 15 bits?
		return TW->PAGE[(ty << 10) + tx];
	}

	return 0;
}

u16 RasterPSXSW::GetTexel3(vectk *v, float s, float t, s16 x, s16 y)
{
	float st = 1.0f - s - t;
	float U = v[0].u * st + v[1].u * s + v[2].u * t + 0.5f;
	float V = v[0].v * st + v[1].v * s + v[2].v * t + 0.5f;
	
	return GetTexel((u8)U, (u8)V);
}

void RasterPSXSW::RasterLine(RENDERTYPE render_mode)
{
	// THE EXTREMELY FAST LINE ALGORITHM Variation D (Addition Fixed Point)
	// http://www.edepot.com/algorithm.html

	bool yLonger=false;
	int incrementVal;

	int shortLen = vertex[1].y - vertex[0].y;
	int longLen = vertex[1].x - vertex[0].x;

	if(longLen > 1023 || shortLen > 511) return;

	if (abs(shortLen)>abs(longLen))
	{
		int swap=shortLen;
		shortLen=longLen;
		longLen=swap;
		yLonger=true;
	}

	incrementVal = 1;
	const int endVal = longLen;

	if(longLen < 0)
	{
		incrementVal = -1;
		longLen = -longLen;
	}

	const int decInc = (!longLen)? 0 : (shortLen << 16) / longLen;

	if(render_mode == RENDER_SHADED)
	{
		RGBA8 c0(vertex[0].c), c1(vertex[1].c);

		const u16 cdr = (!longLen)? 0 : ((c1.R - c0.R) << 8) / longLen;
		const u16 cdg = (!longLen)? 0 : ((c1.G - c0.G) << 8) / longLen;
		const u16 cdb = (!longLen)? 0 : ((c1.B - c0.B) << 8) / longLen;

		u16 cr = 0, cg = 0, cb = 0;

		if (yLonger) for (int i=0, j=0; i!=endVal; i+=incrementVal, j+=decInc)
		{
			c1.R = c0.R + (cr >> 8);
			c1.G = c0.G + (cg >> 8);
			c1.B = c0.B + (cb >> 8);

			SetPixel<false>(c1.RAW, vertex[0].x+(j>>16), vertex[0].y+i, 0);

			cr += cdr;
			cg += cdg;
			cb += cdb;
		}
		else for (int i=0, j=0; i!=endVal; i+=incrementVal, j+=decInc)
		{
			c1.R = c0.R + (cr >> 8);
			c1.G = c0.G + (cg >> 8);
			c1.B = c0.B + (cb >> 8);

			SetPixel<false>(c1.RAW,  vertex[0].x+i, vertex[0].y+(j>>16), 0);

			cr += cdr;
			cg += cdg;
			cb += cdb;
		}
	}
	else
	{
		if (yLonger) for (int i=0, j=0; i!=endVal; i+=incrementVal, j+=decInc)
		{
			SetPixel<false>(vertex[0].c, vertex[0].x+(j>>16), vertex[0].y+i, 0);
		}
		else for (int i=0, j=0; i!=endVal; i+=incrementVal, j+=decInc)
		{
			SetPixel<false>(vertex[0].c, vertex[0].x+i, vertex[0].y+(j>>16), 0);
		}
	}
}

// Copy-pasted from Nick's optimized raster 
// http://devmaster.net/posts/6145/advanced-rasterization

void RasterPSXSW::RasterPoly3(RENDERTYPE render_mode)
{
	s16 minx = min(min(vertex[0].x, vertex[1].x), vertex[2].x);
	s16 maxx = max(max(vertex[0].x, vertex[1].x), vertex[2].x);
	s16 miny = min(min(vertex[0].y, vertex[1].y), vertex[2].y);
	s16 maxy = max(max(vertex[0].y, vertex[1].y), vertex[2].y);

	const u16 width  = maxx - minx;
	const u16 height = maxy - miny;

	if(width > 1023) return;
	if(height > 511) return;

	DA->PolyAreaClip(minx, maxx, miny, maxy);

	const int STDX1 = vertex[1].x - vertex[0].x;
	const int STDX2 = vertex[2].x - vertex[0].x;
	const int STDY1 = vertex[1].y - vertex[0].y;
	const int STDY2 = vertex[2].y - vertex[0].y;

	const int CROSS = (STDX2 * STDY1) - (STDY2 * STDX1);
	if(!CROSS) return;

	const int CSDYX = vertex[0].y * STDX2 - vertex[0].x * STDY2;
	const int CTDYX = vertex[0].y * STDX1 - vertex[0].x * STDY1;

	const float CSY = (float)STDY2 / -CROSS;
	const float CSX = (float)STDX2 / -CROSS;
	const float CS  = (float)CSDYX / -CROSS;

	const float CTY = (float)STDY1 /  CROSS;
	const float CTX = (float)STDX1 /  CROSS;
	const float CT  = (float)CTDYX /  CROSS;

	u16 texel;
	float s, t;

	u8 iA = 0, iB = 1, iC = 2;
	if(CROSS < 0) { iA = 2; iC = 0; }

	// 28.4 fixed-point coordinates
	const int Y1 = vertex[iA].y << 4;
	const int Y2 = vertex[iB].y << 4;
	const int Y3 = vertex[iC].y << 4;

	const int X1 = vertex[iA].x << 4;
	const int X2 = vertex[iB].x << 4;
	const int X3 = vertex[iC].x << 4;

	// Deltas
	const int DX12 = X1 - X2;
	const int DX23 = X2 - X3;
	const int DX31 = X3 - X1;

	const int DY12 = Y1 - Y2;
	const int DY23 = Y2 - Y3;
	const int DY31 = Y3 - Y1;

	// Fixed-point deltas
	const int FDX12 = DX12 << 4;
	const int FDX23 = DX23 << 4;
	const int FDX31 = DX31 << 4;

	const int FDY12 = DY12 << 4;
	const int FDY23 = DY23 << 4;
	const int FDY31 = DY31 << 4;

	// Half-edge constants
	int C1 = DY12 * X1 - DX12 * Y1;
	int C2 = DY23 * X2 - DX23 * Y2;
	int C3 = DY31 * X3 - DX31 * Y3;

	// Correct for fill convention
	if(DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
	if(DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
	if(DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;

	int CY1 = C1 + DX12 * (miny << 4) - DY12 * (minx << 4);
	int CY2 = C2 + DX23 * (miny << 4) - DY23 * (minx << 4);
	int CY3 = C3 + DX31 * (miny << 4) - DY31 * (minx << 4);

	int CX1, CX2, CX3;

	switch(render_mode)
	{
	case RENDER_FLAT:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0)
				{
					SetPixel<false>(vertex[0].c, x, y, 0);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY31;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX31;
		}
		break;

	case RENDER_TEXTURED:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0)
				{
					s = x * CSY - y * CSX + CS;
					t = x * CTY - y * CTX + CT;

				   texel = GetTexel3(vertex, s, t, x, y);

					if(texel)
						SetPixel<true>(vertex[0].c, x, y, texel);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY31;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX31;
		}
		break;

	case RENDER_SHADED:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0)
				{
					s = x * CSY - y * CSX + CS;
					t = x * CTY - y * CTX + CT;

				   SetPixel<false>(GetColorBlend3(vertex, s, t), x, y, 0);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY31;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX31;
		}
		break;

	case RENDER_SHADED_TEXTURED:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0)
				{
					s = x * CSY - y * CSX + CS;
					t = x * CTY - y * CTX + CT;

					texel = GetTexel3(vertex, s, t, x, y);
				
					if(texel)
						SetPixel<true>(GetColorBlend3(vertex, s, t), x, y, texel);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY31;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX31;
		}
		break;
	}
}

void RasterPSXSW::RasterPoly4(RENDERTYPE render_mode)
{
#if 1
	RasterPoly3(render_mode);

	if((render_mode == RENDER_FLAT) || (render_mode == RENDER_TEXTURED))
		vertex[1].c = vertex[0].c;

	vertex[0] = vertex[1];
	vertex[1] = vertex[2];
	vertex[2] = vertex[3];

	RasterPoly3(render_mode);
#else
	s16 minx = min(min(vertex[0].x, vertex[1].x), min(vertex[2].x, vertex[3].x));
	s16 maxx = max(max(vertex[0].x, vertex[1].x), max(vertex[2].x, vertex[3].x));
	s16 miny = min(min(vertex[0].y, vertex[1].y), min(vertex[2].y, vertex[3].y));
	s16 maxy = max(max(vertex[0].y, vertex[1].y), max(vertex[2].y, vertex[3].y));

	const u16 width  = maxx - minx;
	const u16 height = maxy - miny;

	if(width > 1023) return;
	if(height > 511) return;

	DA->PolyAreaClip(minx, maxx, miny, maxy);

	const int STDX1 = vertex[1].x - vertex[0].x;
	const int STDX2 = vertex[2].x - vertex[0].x;
	const int STDY1 = vertex[1].y - vertex[0].y;
	const int STDY2 = vertex[2].y - vertex[0].y;

	const int CROSS = (STDX2 * STDY1) - (STDY2 * STDX1);
	if(!CROSS) return;

	const int CSDYX = vertex[0].y * STDX2 - vertex[0].x * STDY2;
	const int CTDYX = vertex[0].y * STDX1 - vertex[0].x * STDY1;

	const float CSY = (float)STDY2 / -CROSS;
	const float CSX = (float)STDX2 / -CROSS;
	const float CS  = (float)CSDYX / -CROSS;

	const float CTY = (float)STDY1 /  CROSS;
	const float CTX = (float)STDX1 /  CROSS;
	const float CT  = (float)CTDYX /  CROSS;

	u16 texel;
	float s, t;

	u8 iA = 0, iB = 2, iC = 3, iD = 1;
	if(CROSS > 0) { iB = 1; iD = 2; }

	// 28.4 fixed-point coordinates
	const int Y1 = vertex[iA].y << 4;
	const int Y2 = vertex[iB].y << 4;
	const int Y3 = vertex[iC].y << 4;
	const int Y4 = vertex[iD].y << 4;

	const int X1 = vertex[iA].x << 4;
	const int X2 = vertex[iB].x << 4;
	const int X3 = vertex[iC].x << 4;
	const int X4 = vertex[iD].x << 4;

	// Deltas
	const int DX12 = X1 - X2;
	const int DX23 = X2 - X3;
	const int DX34 = X3 - X4;
	const int DX41 = X4 - X1;

	const int DY12 = Y1 - Y2;
	const int DY23 = Y2 - Y3;
	const int DY34 = Y3 - Y4;
	const int DY41 = Y4 - Y1;

	// Fixed-point deltas
	const int FDX12 = DX12 << 4;
	const int FDX23 = DX23 << 4;
	const int FDX34 = DX34 << 4;
	const int FDX41 = DX41 << 4;

	const int FDY12 = DY12 << 4;
	const int FDY23 = DY23 << 4;
	const int FDY34 = DY34 << 4;
	const int FDY41 = DY41 << 4;

	// Half-edge constants
	int C1 = DY12 * X1 - DX12 * Y1;
	int C2 = DY23 * X2 - DX23 * Y2;
	int C3 = DY34 * X3 - DX34 * Y3;
	int C4 = DY41 * X4 - DX41 * Y4;

	// Correct for fill convention
	if(DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
	if(DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
	if(DY34 < 0 || (DY34 == 0 && DX34 > 0)) C3++;
	if(DY41 < 0 || (DY41 == 0 && DX41 > 0)) C4++;

	int CY1 = C1 + DX12 * (miny << 4) - DY12 * (minx << 4);
	int CY2 = C2 + DX23 * (miny << 4) - DY23 * (minx << 4);
	int CY3 = C3 + DX34 * (miny << 4) - DY34 * (minx << 4);
	int CY4 = C4 + DX41 * (miny << 4) - DY41 * (minx << 4);

	int CX1, CX2, CX3, CX4;

	switch(render_mode)
	{
	case RENDER_FLAT:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;
			CX4 = CY4;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0 && CX4 > 0)
				{
					SetPixel<false>(vertex[0].c, x, y, 0);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY34;
				CX4 -= FDY41;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX34;
			CY4 += FDX41;
		}
		break;

	case RENDER_TEXTURED:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;
			CX4 = CY4;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0 && CX4 > 0)
				{
					s = x * CSY - y * CSX + CS;
					t = x * CTY - y * CTX + CT;

				   texel = GetTexel3(vertex, s, t);

					if(texel)
						SetPixel<true>(vertex[0].c, x, y, texel);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY34;
				CX4 -= FDY41;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX34;
			CY4 += FDX41;
		}
		break;

	case RENDER_SHADED:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;
			CX4 = CY4;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0 && CX4 > 0)
				{
					s = x * CSY - y * CSX + CS;
					t = x * CTY - y * CTX + CT;

				   SetPixel<false>(GetColorBlend3(vertex, s, t), x, y, 0);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY34;
				CX4 -= FDY41;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX34;
			CY4 += FDX41;
		}
		break;

	case RENDER_SHADED_TEXTURED:
		for(s16 y = miny; y < maxy; y++)
		{
			CX1 = CY1;
			CX2 = CY2;
			CX3 = CY3;
			CX4 = CY4;

			for(s16 x = minx; x < maxx; x++)
			{
				if(CX1 > 0 && CX2 > 0 && CX3 > 0 && CX4 > 0)
				{
					s = x * CSY - y * CSX + CS;
					t = x * CTY - y * CTX + CT;

					texel = GetTexel3(vertex, s, t);
				
					if(texel)
						SetPixel<true>(GetColorBlend3(vertex, s, t), x, y, texel);
				}

				CX1 -= FDY12;
				CX2 -= FDY23;
				CX3 -= FDY34;
				CX4 -= FDY41;
			}

			CY1 += FDX12;
			CY2 += FDX23;
			CY3 += FDX34;
			CY4 += FDX41;
		}
		break;
	}
#endif
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
					SetPixel<true>(vertex[0].c, x, y, texel);
			}
		}
		else
		{
			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel<false>(vertex[0].c, x, y, 0);
		}
		break;

	case 1: // 1x1
		if(render_mode == RENDER_TEXTURED)
		{
			u16 texel = GetTexel(vertex[0].u, vertex[0].v);

			if(texel)
				SetPixel<true>(vertex[0].c, vertex[0].x, vertex[0].y, texel);
		}
		else
		{
			SetPixel<false>(vertex[0].c, vertex[0].x, vertex[0].y, 0);
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
					SetPixel<true>(vertex[0].c, x, y, texel);
			}
		}
		else
		{
			xmin = vertex[0].x; xmax = xmin + 8;
			ymin = vertex[0].y; ymax = ymin + 8;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel<false>(vertex[0].c, x, y, 0);
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
					SetPixel<true>(vertex[0].c, x, y, texel);
			}
		}
		else
		{
			xmin = vertex[0].x; xmax = xmin + 16;
			ymin = vertex[0].y; ymax = ymin + 16;

			for(y = ymin; y < ymax; y++)
			for(x = xmin; x < xmax; x++)
				SetPixel<false>(vertex[0].c, x, y, 0);
		}
		break;
	}
}


void RasterPSXSW::DrawFill()
{
	RGBA8 pin(vertex[0].c);

	RGBA5551 pix16(0);
	pix16.R = pin.R >> 3;
	pix16.G = pin.G >> 3;
	pix16.B = pin.B >> 3;

	const u16 color16 = pix16.RAW;

	const u16 xmax =  TR->start.U16[0] + TR->size.U16[0];
	const u16 ymax =  TR->start.U16[1] + TR->size.U16[1];

	const u16 xmin = TR->start.U16[0];
	const u16 ymin = TR->start.U16[1];

	for(u16 y = ymin; y < ymax; y++)
	for(u16 x = xmin; x < xmax; x++)
		VRAM->HALF2[y][x] = color16;
}
