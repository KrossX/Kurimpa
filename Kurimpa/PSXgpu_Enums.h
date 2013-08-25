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

// from http://nocash.emubase.de/psx-spx.htm

#pragma once

enum GPU1
{
	GP1_RESET_GPU          = 0x00,
	GP1_RESET_CBUFF        = 0x01,
	GP1_ACK_IRQ            = 0x02,
	GP1_DISPLAY_DISABLE    = 0x03,
	GP1_DMA_DIR_REQ        = 0x04,
	GP1_DISPLAY_START      = 0x05,
	GP1_DISPLAY_HRANGE     = 0x06,
	GP1_DISPLAY_VRANGE     = 0x07,
	GP1_DISPLAY_MODE       = 0x08,
	GP1_TEXTURE_DISABLE    = 0x09,
	GP1_GET_INFO           = 0x10 // 0x10 to 0x1F

	//40 ... 0xFF, mirrors...
};

enum DMAMODE
{
	DMAMODE_OFF         = 0x00,
	DMAMODE_UNKNOWN     = 0x01,
	DMAMODE_CPU2GP0     = 0x02,
	DMAMODE_GPUREAD2CPU = 0x03
};

enum GPUSTAT
{
	// TEXPAGE
	GPUSTAT_TPAGEX      = 0xF,     // 00 (N*64)
	GPUSTAT_TPAGEY      = 1 << 4,  // 04 (N*256) (ie. 0 or 256)
	GPUSTAT_BLENDEQ     = 3 << 5,  // 05 (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)
	GPUSTAT_PAGECOL     = 3 << 7,  // 07 (0=4bit, 1=8bit, 2=15bit, 3=Reserved)
	GPUSTAT_DITHER      = 1 << 9,  // 09 24 to 15bits (0=Off/strip LSBs, 1=Dither Enabled)
	GPUSTAT_DRAWDISPLAY = 1 << 10, // 10 (0=Prohibited, 1=Allowed)
	GPUSTAT_MASKSET     = 1 << 11, // 11 (0=No, 1=Yes/Mask)
	GPUSTAT_MASKCHECK   = 1 << 12, // 12 (0=Always, 1=Not to Masked areas)
	GPUSTAT_RESERVED    = 1 << 13, // 13 reserved, always set?
	GPUSTAT_REVERSED    = 1 << 14, // 14 "reversed flag"
	GPUSTAT_TEXDISABLE  = 1 << 15, // 15 (0=Normal, 1=Disable Textures)

	GPUSTAT_HRES2       = 1 << 16, // 16 (0=256/320/512/640, 1=368)
	GPUSTAT_HRES1       = 3 << 17, // 17 (0=256, 1=320, 2=512, 3=640)
	GPUSTAT_VRES        = 1 << 19, // 19 (0=240, 1=480, when Bit22=1)
	GPUSTAT_VMODE       = 1 << 20, // 20 (0=NTSC/60Hz, 1=PAL/50Hz)
	GPUSTAT_DISPCOLOR   = 1 << 21, // 21 (0=15bit, 1=24bit)
	GPUSTAT_INTERLACED  = 1 << 22, // 22 (0=Off, 1=On)
	GPUSTAT_DISPDISABLE = 1 << 23, // 23 (0=Enabled, 1=Disabled)
	GPUSTAT_IRQ1        = 1 << 24, // 24 (0=Off, 1=IRQ)
	GPUSTAT_DMA         = 1 << 25, // 25 DMA / Data Request

	GPUSTAT_READYCMD    = 1 << 26, // 26 Ready to receive command (0=No, 1=Ready)
	GPUSTAT_READYVRAMCPU= 1 << 27, // 27 (0=No, 1=Ready)
	GPUSTAT_READYDMA    = 1 << 28, // 28 Ready to receive DMA Block (0=No, 1=Ready)
	GPUSTAT_DMADIR      = 3 << 29, // 29(0=Off, 1=?, 2=CPUtoGP0, 3=GPUREADtoCPU)
	GPUSTAT_DRAWLINE    = 1 << 31, // 31(0=Even or Vblank, 1=Odd)
};

enum DCMD
{
	DCMD_ENVREPLACE   = 1,      // 00 Enviromental 1:Replace 0:Modulate
	DCMD_BLENDENABLED = 1 << 1, // 01 Blending 1: Enabled, 0: Disabled
	DCMD_TEXTURED     = 1 << 2, // 02 Texture mapping
	DCMD_QUAD         = 1 << 3, // 03 Vertex count, STRIP for lines
	DCMD_GOURAUD      = 1 << 4, // 04 Gouraud shading
	DCMD_SIZE         = 3 << 3, // 03 (RECT) Size 0: variable, 1: 1x1, 2: 8x8, 3: 16x16
	DCMD_TYPE         = 3 << 5, // 04 1: Poly, 2: Line, 3: Rect
};

enum GPUMODE
{
	GPUMODE_COMMAND,
	GPUMODE_DRAW_POLY3,
	GPUMODE_DRAW_POLY3T,
	GPUMODE_DRAW_POLY3S,
	GPUMODE_DRAW_POLY3ST,
	GPUMODE_DRAW_POLY4,
	GPUMODE_DRAW_POLY4T,
	GPUMODE_DRAW_POLY4S,
	GPUMODE_DRAW_POLY4ST,
	GPUMODE_DRAW_LINE,
	GPUMODE_DRAW_LINES,
	GPUMODE_DRAW_LINEG,
	GPUMODE_DRAW_LINEGS,
	GPUMODE_DRAW_RECT0,
	GPUMODE_DRAW_RECT0T,
	GPUMODE_DRAW_RECT1,
	GPUMODE_DRAW_RECT1T,
	GPUMODE_DRAW_RECT8,
	GPUMODE_DRAW_RECT8T,
	GPUMODE_DRAW_RECT16,
	GPUMODE_DRAW_RECT16T,
	GPUMODE_FILL,
	GPUMODE_TRANSFER_VRAM_VRAM,
	GPUMODE_TRANSFER_CPU_VRAM,
	GPUMODE_TRANSFER_VRAM_CPU,
	GPUMODE_DUMMY
};

enum RENDERTYPE
{
	RENDER_FLAT,
	RENDER_TEXTURED,
	RENDER_SHADED,
	RENDER_SHADED_TEXTURED
};