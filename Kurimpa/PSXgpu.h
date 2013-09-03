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

struct SGPUSTAT
{
	// TEXPAGE
	u8 TPAGEX;      // 00-4bits (N*64)
	u8 TPAGEY;      // 04 (N*256) (ie. 0 or 256)
	u8 BLENDEQ;     // 05-2bits (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)
	u8 PAGECOL;     // 07-2bits (0=4bit, 1=8bit, 2=15bit, 3=Reserved)
	u8 DITHER;      // 09 24 to 15bits (0=Off/strip LSBs, 1=Dither Enabled)
	u8 DRAWDISPLAY; // 10 (0=Prohibited, 1=Allowed)
	u8 MASKSET;     // 11 (0=No, 1=Yes/Mask)
	u8 MASKCHECK;   // 12 (0=Always, 1=Not to Masked areas)
	u8 RESERVED;    // 13 reserved, always set?
	u8 REVERSED;    // 14 "reversed flag"
	u8 TEXDISABLE;  // 15 (0=Normal, 1=Disable Textures)

	u8 TXPDISABLE; // ??
	u8 RECT_FLIPX; // ??
	u8 RECT_FLIPY; // ??

	u8 HRES2;       // 16 (0=256/320/512/640, 1=368)
	u8 HRES1;       // 17-2bits (0=256, 1=320, 2=512, 3=640)
	u8 VRES;        // 19 (0=240, 1=480, when Bit22=1)
	u8 VMODE;       // 20 (0=NTSC/60Hz, 1=PAL/50Hz)
	u8 DISPCOLOR;   // 21 (0=15bit, 1=24bit)
	u8 INTERLACED;  // 22 (0=Off, 1=On)
	u8 DISPDISABLE; // 23 (0=Enabled, 1=Disabled)
	u8 IRQ1;        // 24 (0=Off, 1=IRQ)
	u8 DMA;         // 25 DMA / Data Request

	u8 READYCMD;     // 26 Ready to receive command (0=No, 1=Ready) // Wait finish drawing primites (1) || isDrawing wait
	u8 READYVRAMCPU; // 27 (0=No, 1=Ready)
	u8 READYDMA;     // 28 Ready to receive DMA Block (0=No, 1=Ready) // Wait for gpu to be free (1)
	u8 DMADIR;       // 29-2bits (0=Off, 1=?, 2=CPUtoGP0, 3=GPUREADtoCPU)
	u8 DRAWLINE;     // 31 (0=Even or Vblank, 1=Odd)

	u32 GetU32();
	void SetU32(u32 reg);
	void SetTEXPAGE(u16 PAGE);
};

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

	inline void SetPAGE(u16 VRAM[][1024], SGPUSTAT &GPUSTAT);
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

	void PolyAreaClip(s16 &minX, s16 &maxX, s16 &minY, s16 &maxY)
	{
		minX = max(minX, L - OFFx);
		maxX = min(maxX, R - OFFx);
		minY = max(minY, T - OFFy);
		maxY = min(maxY, B - OFFy);
	}
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

	bool is480i;
	bool isPAL;
	bool isInterlaced; 
	bool is24bpp; // 15b or 24b color mode

	s32 rangeh, rangev; // GP1(06h-07h)
	s32 cycles; // to get width

	u16 X1, X2, Y1, Y2;
	u8 scanlines; // 224 NTSC, 264 PAL

	bool changed;

	void SetMode(SGPUSTAT &REG)
	{
		if(REG.HRES2)
		{
			width = 368;
			cycles = 7;
		}
		else switch(REG.HRES1)
		{
		case 0: width = 256; cycles = 10; break;
		case 1: width = 320; cycles = 8; break;
		case 2: width = 512; cycles = 5; break;
		case 3: width = 640; cycles = 4; break;
		}

		isPAL = !!REG.VMODE;
		is24bpp = !!REG.DISPCOLOR;
		isInterlaced = !!REG.INTERLACED;

		height = (REG.VRES && isInterlaced) ? 480 : 240;
		scanlines = isPAL ? 264 : 224; // PAL 255, 254 ??
		is480i = height == 480;

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

union PSXVRAM
{
	u8 BYTE1[0x100000];
	u16 HALF2[0x200][0x400]; // Y, X
};

struct RasterPSX;
struct RasterPSXSW;
class RenderOGL_PSX;

struct PSXgpu
{
	HINSTANCE hInstance;
	LPCWSTR applicationName;
	HWND hWnd;

	u8 GPUMODE;

	SGPUSTAT GPUSTAT;
	u32 GPUREAD;

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

	PSXVRAM VRAM;

	bool doMaskCheck(u16 &x, u16 &y) { return GPUSTAT.MASKCHECK && (VRAM.HALF2[y][x] & 0x8000); }
	bool doMaskCheck(u16 &pix) { return GPUSTAT.MASKCHECK && (pix & 0x8000); }
	
	u32 gpcount;
	u32 gpsize;

	void SetReady()
	{
		//SetReadyCMD(1);
		//SetReadyDMA(1);
		gpcount = 0;
		GPUMODE = GPUMODE_COMMAND;
	}

	void Reset();
	void Command(u32 data);

	RenderOGL_PSX *render;
	RasterPSX *raster;
	RasterPSXSW *rasterSW;

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

	int DmaChain(u32 *base, u32 addr);

	void SaveState(u32 &STATUS, u32 *CTRL, u8 *MEM);
	void LoadState(u32 &STATUS, u32 *CTRL, u8 *MEM);
};

