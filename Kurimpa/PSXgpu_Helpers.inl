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

u32 SGPUSTAT::GetU32()
{
	u32 GPUSTATRAW = 0;

	GPUSTATRAW |=  TPAGEX&0xF;
	GPUSTATRAW |= (TPAGEY&1)      << 4;
	GPUSTATRAW |= (BLENDEQ&3)     << 5;
	GPUSTATRAW |= (PAGECOL&3)     << 7;
	GPUSTATRAW |= (DITHER&1)      << 9;
	GPUSTATRAW |= (DRAWDISPLAY&1) <<10;
	GPUSTATRAW |= (MASKSET&1)     <<11;
	GPUSTATRAW |= (MASKCHECK&1)   <<12;
	GPUSTATRAW |= (RESERVED&1)    <<13;
	GPUSTATRAW |= (REVERSED&1)    <<14;
	GPUSTATRAW |= (TEXDISABLE&1)  <<15;
	GPUSTATRAW |= (HRES2&1)       <<16;
	GPUSTATRAW |= (HRES1&3)       <<17;
	GPUSTATRAW |= (VRES&1)        <<19;
	GPUSTATRAW |= (VMODE&1)       <<20;
	GPUSTATRAW |= (DISPCOLOR&1)   <<21;
	GPUSTATRAW |= (INTERLACED&1)  <<22;
	GPUSTATRAW |= (DISPDISABLE&1) <<23;
	GPUSTATRAW |= (IRQ1&1)        <<24;
	GPUSTATRAW |= (DMA&1)         <<25;
	GPUSTATRAW |= (READYCMD&1)    <<26;
	GPUSTATRAW |= (READYVRAMCPU&1)<<27;
	GPUSTATRAW |= (READYDMA&1)    <<28;
	GPUSTATRAW |= (DMADIR&3)      <<29;
	GPUSTATRAW |= (DRAWLINE&1)    <<31;

	return GPUSTATRAW;
}

void SGPUSTAT::SetU32(u32 reg)
{
}

void SGPUSTAT::SetTEXPAGE(u16 PAGE)
{
	TPAGEX      =  PAGE&0xF;
	TPAGEY      = (PAGE>>4)&1;
	BLENDEQ     = (PAGE>>5)&3;
	PAGECOL     = (PAGE>>7)&3;
	//DITHER      = (PAGE>>9)&1;
	//DRAWDISPLAY = (PAGE>>10)&1;
	TXPDISABLE  = (PAGE>>11)&1;
	//RECT_FLIPX  = (PAGE>>12)&1;
	//RECT_FLIPY  = (PAGE>>13)&1;
}


inline void TEXWIN::SetMaskOffset(u32 data)
{
	MSKx = data & 0x1F;
	MSKy = (data >> 5) & 0x1F;
	OFFx = (data >> 10) & 0x1F;
	OFFy = (data >> 15) & 0x1F;

	OFFx = (OFFx & MSKx) << 3;
	OFFy = (OFFy & MSKy) << 3;
	MSKx = ~(MSKx << 3);
	MSKy = ~(MSKy << 3);
}

inline void TEXWIN::SetPAGE(u16 VRAM[][1024], SGPUSTAT &GPUSTAT)
{
	u16 PX = GPUSTAT.TPAGEX;
	u16 PY = GPUSTAT.TPAGEY;
	PAGE = &VRAM[PY << 8][PX << 6];
}

inline void TEXWIN::SetCLUT(u16 VRAM[][1024], u16 &data)
{
	CLUT = &VRAM[(data >> 6) & 0x1FF][(data & 0x3F) << 4];
}

inline void DRAWAREA::SetTL(u32 data)
{
	L = data & 0x3FF;
	T = (data >> 10) & 0x3FF;
}

inline void DRAWAREA::SetBR(u32 data)
{
	R = data & 0x3FF;
	B = (data >> 10) & 0x3FF;
}

inline void DRAWAREA::SetOFFSET(u32 data)
{
	u16 ox = (data & 0x7FF) << 5;
	u16 oy = ((data >> 11) & 0x7FF) << 5;
	OFFx = (s16)ox; OFFx >>= 5;
	OFFy = (s16)oy; OFFy >>= 5;
}


inline void DRAWAREA::PolyAreaClip(s16 &minX, s16 &maxX, s16 &minY, s16 &maxY)
{
	minX = max(minX, L - OFFx);
	maxX = min(maxX, R - OFFx);
	minY = max(minY, T - OFFy);
	maxY = min(maxY, B - OFFy);
}
