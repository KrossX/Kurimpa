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

union CommandPacket
{
	struct
	{
		u32 parameter:24;
		u32 command:8;
	};
	
	u32 RAW;

	CommandPacket() {};
	CommandPacket(u32 data) : RAW(data) {};
	CommandPacket(u8 cmd, u32 data) : RAW((cmd << 24) | (data & 0xFFFFFF)) {};
};

struct TEXWIN
{
	u8 MSKx, MSKy;
	u8 OFFx, OFFy;

	u16 *CLUT, *PAGE;

	inline void SetPAGE(u16 VRAM[][1024], u32 &GPUSTAT);
	inline void SetCLUT(u16 VRAM[][1024], u16 &data);
	inline void SetMaskOffset(u32 data);

	void ApplyMaskOffset(u8 &tx, u8 &ty)
	{
		tx = (tx & MSKx) | OFFx;
		ty = (ty & MSKy) | OFFy;
	}
};

struct DRAWAREA
{
	s16 T, L, B, R;
	s16 OFFx, OFFy;

	inline void SetTL(u32 data);
	inline void SetBR(u32 data);
	inline void SetOFFSET(u32 data);

	void ApplyOffset(s16 &x, s16 &y) { x += OFFx; y += OFFy; }
	bool ScissorTest(s16 &x, s16 &y) { return (x > R) || (y > B) || (x < L) || (y < T); }

	inline void PolyAreaClip(s16 &minX, s16 &maxX, s16 &minY, s16 &maxY);
};

struct TRANSFER
{
	Word src, dst;
	Word start, size;
	u32 total;

	void SetSize(u32 data)
	{
		size.U32 = data;
		size.U16[0] =  ((size.U16[0] - 1) & 0x3FF) + 1;
		size.U16[1] =  ((size.U16[1] - 1) & 0x1FF) + 1;
		total = size.U16[0] * size.U16[1];
	}

	u32 GetSize()
	{
		return (total + total%2) / 2  + 2;
	}
};

struct DISPINFO
{
	s32 ox, oy; // origin XY
	s32 cx, cy; // offset XY
	s32 width, height;

	bool isPAL;
	bool isInterlaced; 
	bool is24bpp; // 15b or 24b color mode

	s32 rangeh, rangev; // GP1(06h-07h)
	s32 cycles; // to get width

	u16 X1, X2, Y1, Y2;
	u8 scanlines; // 224 NTSC, 264 PAL

	bool changed;

	void SetMode(u32 REG)
	{
		u8 hres2 = ((REG & GPUSTAT_HRES1) >> 17) &3;

		if(REG & GPUSTAT_HRES2)
		{
			width = 368;
			cycles = 7;
		}
		else switch(hres2)
		{
		case 0: width = 256; cycles = 10; break;
		case 1: width = 320; cycles = 8; break;
		case 2: width = 512; cycles = 5; break;
		case 3: width = 640; cycles = 4; break;
		}

		isPAL = !!(REG & GPUSTAT_VMODE);
		is24bpp = !!(REG & GPUSTAT_DISPCOLOR);
		isInterlaced = !!(REG & GPUSTAT_INTERLACED);

		height = ((REG & GPUSTAT_VRES) && isInterlaced) ? 480 : 240;
		scanlines = isPAL ? 264 : 224;

		changed = true;
	}

	void UpdateCentering()
	{
		if(!changed) return;

		rangeh = (X2 - X1) / cycles;
		//cx = (X1 - 0x260) / cycles;
		cx = (width - rangeh) / 2;

		/*
		if(isPAL)
			cy = (Y1 - 0x1F);
		else
			cy = (Y1 - 0x18);
		*/

		rangev = (Y2 - Y1);
		cy  = (240 - rangev) / 2;

		cy     *= height == 480 ? 2 : 1;
		rangev *= height == 480 ? 2 : 1;

		changed = false;
	}

	void SetSTART(u32 data)
	{
		ox = data & 0x3FF;
		oy = (data >> 10) & 0x1FF;
	}

	void SetHRANGE(u32 data)
	{
		X1 = data & 0xFFF;
		X2 = (data >> 12) & 0xFFF;

		changed = true;
	}

	void SetVRANGE(u32 data)
	{
		Y1 = data & 0x3FF;
		Y2 = (data >> 10) & 0x3FF;

		changed = true;
	}
	
};

struct PSXgpu
{
	HINSTANCE hInstance;
	LPCWSTR applicationName;
	HWND hWnd;

	u8 GPUMODE;

	u32 GPUREAD;
	u32 GPUSTAT;

	u32 DISP_START;  // GP1(05h);
	u32 DISP_RANGEH; // GP1(06h);
	u32 DISP_RANGEV; // GP1(07h);

	u32 TEXTURE_WINDOW; // GP0(E2h);
	u32 DRAW_AREA_TL;   // GP0(E3h);
	u32 DRAW_AREA_BR;   // GP0(E4h);
	u32 DRAW_OFFSET;    // GP0(E3h);

	TEXWIN   TW; // Texture Window
	DRAWAREA DA; // Drawing Area
	TRANSFER TR; // Transfer Stiff
	DISPINFO DI; // Display Information
	
	u8 DC; // Drawing Command

	vectk vertex[4];

	union
	{
		u8 BYTE1[0x100000];
		u16 HALF2[0x200][0x400]; // Y, X
	} VRAM;

	void SetTEXPAGE(u16 &data) { GPUSTAT &= ~0x1FF; GPUSTAT |= (data&0x1FF); } // 0x1FF
	void SetMask(u8 bit2) { GPUSTAT &= ~(GPUSTAT_MASKSET | GPUSTAT_MASKCHECK); GPUSTAT |= (bit2&3) << 11; }
	void SetTextureDisable(u8 bit) { GPUSTAT &= ~GPUSTAT_TEXDISABLE; GPUSTAT |= (bit&1) << 15; }
	void SetDisplayDisable(u8 bit) { GPUSTAT &= ~GPUSTAT_DISPDISABLE; GPUSTAT |= (bit&1) << 23; }
	void SetIRQ(u8 bit) { GPUSTAT &= ~GPUSTAT_IRQ1; GPUSTAT |= (bit&1) << 24; }
	void SetDMA(u8 bit) { GPUSTAT &= ~GPUSTAT_DMA; GPUSTAT |= (bit&1) << 25; }
	void SetReadyCMD(u8 bit) { GPUSTAT &= ~GPUSTAT_READYCMD; GPUSTAT |= (bit&1) << 26; }
	void SetReadyVRAMtoCPU(u8 bit) { GPUSTAT &= ~GPUSTAT_READYVRAMCPU; GPUSTAT |= (bit&1) << 27; }
	void SetReadyDMA(u8 bit) { GPUSTAT &= ~GPUSTAT_READYDMA; GPUSTAT |= (bit&1) << 28; }
	void SetDMAdirection(u8 bit2)  { GPUSTAT &= ~GPUSTAT_DMADIR; GPUSTAT |= (bit2&3) << 29; }

	bool doMaskCheck(u16 &x, u16 &y) { return (GPUSTAT & GPUSTAT_MASKCHECK) && (VRAM.HALF2[y][x] & 0x8000); }
	bool doMaskCheck(u16 &pix) { return (GPUSTAT & GPUSTAT_MASKCHECK) && (pix & 0x8000); }
	
	u32 gpcount;
	u32 gpsize;

	void SetReady()
	{
		GPUSTAT |= GPUSTAT_READYCMD;
		gpcount = 0;
		GPUMODE = GPUMODE_COMMAND;
	}

	void Reset();
	void Command(u32 data);

	inline u16 Blend(u16 &back, u16 &front);
	inline void SetPixel(u32 color, s16 x, s16 y, u16 texel);

	template<bool textured, bool modulate, bool blend>
	inline void SetPixel(u32 color, s16 x, s16 y, u16 texel);

	inline u16  GetTexel(u8 tx, u8 ty);
	
	template <RENDERTYPE render_mode> inline void RasterLine();
	template <RENDERTYPE render_mode> inline void RasterPoly3();
	template <RENDERTYPE render_mode> inline void RasterPoly4();

	inline void DrawPoly3  (u32 data);
	inline void DrawPoly3T (u32 data);
	inline void DrawPoly3S (u32 data);
	inline void DrawPoly3ST(u32 data);
	inline void DrawPoly4  (u32 data);
	inline void DrawPoly4T (u32 data);
	inline void DrawPoly4S (u32 data);
	inline void DrawPoly4ST(u32 data);

	inline void DrawLine  (u32 data);
	inline void DrawLineS (u32 data);
	inline void DrawLineG (u32 data);
	inline void DrawLineGS(u32 data);

	inline void DrawRect0  (u32 data);
	inline void DrawRect0T (u32 data);
	inline void DrawRect1  (u32 data);
	inline void DrawRect1T (u32 data);
	inline void DrawRect8  (u32 data);
	inline void DrawRect8T (u32 data);
	inline void DrawRect16 (u32 data);
	inline void DrawRect16T(u32 data);
	
	inline void DrawStart(u32 data);
	inline void DrawFill(u32 data);

	inline void Transfer_VRAM_VRAM(u32 data);
	inline void Transfer_CPU_VRAM(u32 data);
	inline void Transfer_VRAM_CPU(u32 data);

public:
	WNDPROC emuWndProc;


	int Init();
	int Shutdown();
	
	int Open(HWND hGpuWnd);
	int Close();
	
	void TakeSnapshot();
	void LaceUpdate();

	u32 ReadStatus();
	void WriteStatus(u32 data);
	void GP1(u8 cmd, u32 param)	{ WriteStatus((cmd << 24) | (param & 0xFFFFFF)); }
	
	u32 ReadData();
	void WriteData(u32 data);
	void GP0(u8 cmd, u32 param)	{ WriteData((cmd << 24) | (param & 0xFFFFFF)); }
	
	void ReadDataMem(u32 * pMem, int iSize);
	void WriteDataMem(u32 * pMem, int iSize);

	void SetMode(u32 data);
	u32 GetMode();

	int DmaChain(u32 * base, u32 addr);

	void SaveState(u32 &STATUS, u32 *CTRL, u8 *MEM);
	void LoadState(u32 &STATUS, u32 *CTRL, u8 *MEM);
};

