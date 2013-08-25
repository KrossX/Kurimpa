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

inline void TEXWIN::SetPAGE(u16 VRAM[][1024], u32 &GPUSTAT)
{
	u16 PX = GPUSTAT & 0xF;
	u16 PY = (GPUSTAT >> 4) & 1;
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
