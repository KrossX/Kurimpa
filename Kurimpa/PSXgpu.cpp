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
#include "RenderOGL_PSX.h"

#include "PSXgpu_Helpers.inl"
#include "PSXgpu_Draw.inl"
#include "PSXgpu_MemTransfer.inl"

int PSXgpu::Init()
{
	DebugFunc();

	render = NULL;
	raster = NULL;
	rasterSW = NULL;

	Reset();
	return 0;
}

void PSXgpu::Reset()
{
	DebugFunc();
	GPUSTAT.RESERVED = 1; // Always set

	WriteStatus(0x01000000);
	WriteStatus(0x02000000);
	WriteStatus(0x03000001);
	WriteStatus(0x04000000);
	WriteStatus(0x05000000);
	WriteStatus(0x06C00200);
	WriteStatus(0x07040010);
	WriteStatus(0x08000000);
	
	Command(0xE1000000);
	Command(0xE2000000);
	Command(0xE3000000);
	Command(0xE4000000);
	Command(0xE5000000);
	Command(0xE6000000);

	GPUSTAT.READYCMD = 1;
	GPUSTAT.READYDMA = 1;
}

int PSXgpu::Shutdown()
{
	DebugFunc();
	return 0;
}

int PSXgpu::Open(HWND hGpuWnd)
{
	DebugFunc();

	render = new RenderOGL_PSX();
	rasterSW = new RasterPSXSW();

	if(!render || !rasterSW || !render->Init(hGpuWnd, VRAM.BYTE1))
	{
		if(!render)
			printf("Kurimpa -> Error! Could not create renderer.\n");
		else if(!rasterSW)
			printf("Kurimpa -> Error! Could not create SW rasterizer.\n");
		else
			printf("Kurimpa -> Error! Could not Init renderer.\n");

		PostMessage(hGpuWnd, WM_KEYDOWN, VK_ESCAPE, 0);
		Close();
		return -1;
	}

	raster = rasterSW;
	rasterSW->InitPointers(&GPUSTAT, &DA, &VRAM, &TW, vertex, &TR);

	return 0;
}

int PSXgpu::Close()
{
	DebugFunc();

	if(render)
	{
		render->Shutdown();
		delete render;
	}

	if(rasterSW)
	{
		// shutdown?
		delete rasterSW;
	}

	render = NULL;
	raster = NULL;
	rasterSW = NULL;

	return 0;
}

void PSXgpu::TakeSnapshot()
{
	DebugFunc();
}

u32 PSXgpu::ReadStatus() // 0x1F801814 GPUSTAT
{
	if(!DI.is480i) GPUSTAT.DRAWLINE ^= 1;
	u32 GPUSTATRAW = GPUSTAT.GetU32();
	DebugPrint("[%08X]", GPUSTATRAW);
	return GPUSTATRAW;
}

void PSXgpu::WriteStatus(u32 data) // 0x1F801814 GP1
{
	CommandPacket stat(data);
	DebugPrint("[%02X|%06X]", stat.command, stat.parameter);

	switch(stat.command & 0x3F)
	{
	case GP1_RESET_GPU:
		Reset();
		break;

	case GP1_RESET_CBUFF:
		break;

	case GP1_ACK_IRQ:
		GPUSTAT.IRQ1 = 0;
		break;

	case GP1_DISPLAY_DISABLE:
		GPUSTAT.DISPDISABLE = data&1;
		break;

	case GP1_DMA_DIR_REQ:
		GPUSTAT.DMADIR = data&3;

		switch(data&3)
		{
		case 0: GPUSTAT.DMA = 0; break; // Always zero (0)
		case 1: GPUSTAT.DMA = 1; break; // FIFO State  (0=Full, 1=Not Full)
		case 2: GPUSTAT.DMA = GPUSTAT.READYDMA; break;
		case 3: GPUSTAT.DMA = GPUSTAT.READYVRAMCPU; break;
		}
		break;

	case GP1_DISPLAY_START:
		DISP_START = data & 0x7FFFF;
		DI.SetSTART(DISP_START);
		if(render) render->SetDisplayOffset(DI.ox, DI.oy);
		break;

	case GP1_DISPLAY_HRANGE:
		DISP_RANGEH = data & 0xFFFFFF;
		DI.SetHRANGE(DISP_RANGEH);
		break;

	case GP1_DISPLAY_VRANGE:
		DISP_RANGEV = data & 0xFFFFF;
		DI.SetVRANGE(DISP_RANGEV);
		break;

	case GP1_DISPLAY_MODE:
		GPUSTAT.HRES1      =  data&3;
		GPUSTAT.VRES       = (data>>2)&1;
		GPUSTAT.VMODE      = (data>>3)&1;
		GPUSTAT.DISPCOLOR  = (data>>4)&1;
		GPUSTAT.INTERLACED = (data>>5)&1;
		GPUSTAT.HRES2      = (data>>6)&1;
		GPUSTAT.REVERSED   = (data>>7)&1;
		DI.SetMode(GPUSTAT);
		if(render) render->SetDisplayMode(DI.width, DI.height);
		break;

	case GP1_TEXTURE_DISABLE:
		GPUSTAT.TEXDISABLE = data&1;
		break;

	default: if(stat.command & 0x10) // 0x10 to 0x1F
	{
		switch(data & 0xF)
		{
		case 0x2: GPUREAD = TEXTURE_WINDOW; break;
		case 0x3: GPUREAD = DRAW_AREA_TL; break;
		case 0x4: GPUREAD = DRAW_AREA_BR; break;
		case 0x5: GPUREAD = DRAW_OFFSET; break;
		case 0x7: GPUREAD = 0x02; break; // GPU version
		case 0x8: GPUREAD = 0x00; break; // Unknown ?
		}
	}
	break;
	}
}

u32 PSXgpu::ReadData() // 0x1F801810 GPUREAD
{
	if(GPUMODE == GPUMODE_TRANSFER_VRAM_CPU)
	{
		Transfer_VRAM_CPU(0);
		gpcount++;
	}

	DebugPrint("[%08X] (%d)", GPUREAD, gpcount);
	return GPUREAD;
}

void PSXgpu::LaceUpdate()
{
	DebugPrint("--------------------------------------Vsync");

	static u32 framecount = 0;
	static DWORD oldtime = 0;

	DWORD newtime = GetTickCount();
	DWORD diff = newtime - oldtime;

	if(diff > 500)
	{
		char buff[256];
		memset(buff, 0, sizeof(buff));

		float time = (float)(diff) / 1000.0f;

		sprintf_s(buff, "Kurimpa: %s %d%s (%d,%d) | FPS: %1.2f", 
			DI.isPAL ? "PAL" : "NTSC", 
			DI.height, DI.isInterlaced? "i" : "p", 
			DI.width, DI.height,
			//DI.x1, DI.x2, DI.y1, DI.y2,
			framecount / time);

		render->UpdateTitle(buff);

		oldtime = newtime;
		framecount = 0;
	}

	framecount++;

	DI.UpdateCentering();

	// This entire mess is for screen position
	render->SetPSXoffset(DI.cx, DI.ox + DI.rangeh - 1, DI.cy, DI.oy + DI.rangev - 1);
	render->Present(DI.is24bpp, !!GPUSTAT.DISPDISABLE);

	if(DI.is480i) GPUSTAT.DRAWLINE ^= 1;
	else          GPUSTAT.DRAWLINE  = 0; // VBlank
}



void PSXgpu::Command(u32 data) // 0x1F801810 GP0
{
	CommandPacket write(data); gpcount = 0;
	DebugPrint("[%02X|%06X]", write.command, write.parameter);

	switch(write.command)
	{
		case 0x00: break; // NOP?
		case 0x01: SetReady(); break; // Reset Cache
		case 0x02: DrawFill(data); break;
		case 0x03: break; // Pawlov by elitegroup
		//case 0x04: break;
		//case 0x05: break;
		//case 0x06: break;
		//case 0x07: break;
		//case 0x08: break;
		case 0x09: break; // FF8 World
		case 0x0A: break; // FF8 World
		//case 0x0B: break;
		//case 0x0C: break;
		//case 0x0D: break;
		//case 0x0E: break;
		//case 0x0F: break;
		//case 0x10: break;
		//case 0x11: break;
		//case 0x12: break;
		//case 0x13: break;
		//case 0x14: break;
		//case 0x15: break;
		//case 0x16: break;
		//case 0x17: break;
		//case 0x18: break;
		//case 0x19: break;
		//case 0x1A: break;
		//case 0x1B: break;
		//case 0x1C: break;
		//case 0x1D: break;
		//case 0x1E: break;
		case 0x1F:
			GPUSTAT.IRQ1 = 1;
			MessageBoxA(NULL, "IRQ?", "Really?", MB_OK);
			break;

		case 0x20: case 0x21: case 0x22: case 0x23: DrawPoly3  (data); break;
		case 0x24: case 0x25: case 0x26: case 0x27: DrawPoly3T (data); break;
		case 0x28: case 0x29: case 0x2A: case 0x2B: DrawPoly4  (data); break;
		case 0x2C: case 0x2D: case 0x2E: case 0x2F: DrawPoly4T (data); break;
		case 0x30: case 0x31: case 0x32: case 0x33: DrawPoly3S (data); break;
		case 0x34: case 0x35: case 0x36: case 0x37: DrawPoly3ST(data); break;
		case 0x38: case 0x39: case 0x3A: case 0x3B: DrawPoly4S (data); break;
		case 0x3C: case 0x3D: case 0x3E: case 0x3F: DrawPoly4ST(data); break;

		case 0x40: case 0x41: case 0x42: case 0x43: DrawLine  (data); break;
		case 0x44: case 0x45: case 0x46: case 0x47: DrawLine  (data); break;
		case 0x48: case 0x49: case 0x4A: case 0x4B: DrawLineS (data); break;
		case 0x4C: case 0x4D: case 0x4E: case 0x4F: DrawLineS (data); break;
		case 0x50: case 0x51: case 0x52: case 0x53: DrawLineG (data); break;
		case 0x54: case 0x55: case 0x56: case 0x57: DrawLineG (data); break;
		case 0x58: case 0x59: case 0x5A: case 0x5B: DrawLineGS(data); break;
		case 0x5C: case 0x5D: case 0x5E: case 0x5F: DrawLineGS(data); break;

		case 0x60: case 0x61: case 0x62: case 0x63: DrawRect0  (data); break;
		case 0x64: case 0x65: case 0x66: case 0x67: DrawRect0T (data); break;
		case 0x68: case 0x69: case 0x6A: case 0x6B: DrawRect1  (data); break;
		case 0x6C: case 0x6D: case 0x6E: case 0x6F: DrawRect1T (data); break;
		case 0x70: case 0x71: case 0x72: case 0x73: DrawRect8  (data); break;
		case 0x74: case 0x75: case 0x76: case 0x77: DrawRect8T (data); break;
		case 0x78: case 0x79: case 0x7A: case 0x7B: DrawRect16 (data); break;
		case 0x7C: case 0x7D: case 0x7E: case 0x7F: DrawRect16T(data); break;

		case 0x80: case 0x81: case 0x82: case 0x83:
		case 0x84: case 0x85: case 0x86: case 0x87:
		case 0x88: case 0x89: case 0x8A: case 0x8B:
		case 0x8C: case 0x8D: case 0x8E: case 0x8F:
		case 0x90: case 0x91: case 0x92: case 0x93:
		case 0x94: case 0x95: case 0x96: case 0x97:
		case 0x98: case 0x99: case 0x9A: case 0x9B:
		case 0x9C: case 0x9D: case 0x9E: case 0x9F:
			Transfer_VRAM_VRAM(data); break;

		case 0xA0: case 0xA1: case 0xA2: case 0xA3:
		case 0xA4: case 0xA5: case 0xA6: case 0xA7:
		case 0xA8: case 0xA9: case 0xAA: case 0xAB:
		case 0xAC: case 0xAD: case 0xAE: case 0xAF:
		case 0xB0: case 0xB1: case 0xB2: case 0xB3:
		case 0xB4: case 0xB5: case 0xB6: case 0xB7:
		case 0xB8: case 0xB9: case 0xBA: case 0xBB:
		case 0xBC: case 0xBD: case 0xBE: case 0xBF:
			Transfer_CPU_VRAM(data); break;

		case 0xC0: case 0xC1: case 0xC2: case 0xC3:
		case 0xC4: case 0xC5: case 0xC6: case 0xC7:
		case 0xC8: case 0xC9: case 0xCA: case 0xCB:
		case 0xCC: case 0xCD: case 0xCE: case 0xCF:
		case 0xD0: case 0xD1: case 0xD2: case 0xD3:
		case 0xD4: case 0xD5: case 0xD6: case 0xD7:
		case 0xD8: case 0xD9: case 0xDA: case 0xDB:
		case 0xDC: case 0xDD: case 0xDE: case 0xDF:
			Transfer_VRAM_CPU(data); break;

		//case 0xE0: break;

		case 0xE1:
			GPUSTAT.TPAGEX      =  data&0xF;
			GPUSTAT.TPAGEY      = (data>>4)&1;
			GPUSTAT.BLENDEQ     = (data>>5)&3;
			GPUSTAT.PAGECOL     = (data>>7)&3;
			GPUSTAT.DITHER      = (data>>9)&1;
			GPUSTAT.DRAWDISPLAY = (data>>10)&1;
			GPUSTAT.TXPDISABLE  = (data>>11)&1;
			GPUSTAT.RECT_FLIPX  = (data>>12)&1;
			GPUSTAT.RECT_FLIPY  = (data>>13)&1;
			break;

		case 0xE2:
			TEXTURE_WINDOW = data & 0xFFFFF; // 20bits
			TW.SetMaskOffset(TEXTURE_WINDOW);
			break;

		case 0xE3:
			DRAW_AREA_TL = data & 0xFFFFF; // 20bits
			DA.SetTL(DRAW_AREA_TL);
			break;

		case 0xE4:
			DRAW_AREA_BR = data & 0xFFFFF; // 20bits
			DA.SetBR(DRAW_AREA_BR);
			break;

		case 0xE5:
			DRAW_OFFSET = data & 0x3FFFFF; // 22bits
			DA.SetOFFSET(DRAW_OFFSET);
			break;

		case 0xE6:
			GPUSTAT.MASKSET   =  data&1;
			GPUSTAT.MASKCHECK = (data>>1)&1;
			GPUSTAT.MASK16 = GPUSTAT.MASKSET ? 0x8000 : 0x00;
			GPUSTAT.MASK32 = GPUSTAT.MASKSET ? 0x80008000 : 0x00;
			break;

		//case 0xE7: break;
		//case 0xE8: break;
		//case 0xE9: break;
		//case 0xEA: break;
		//case 0xEB: break;
		//case 0xEC: break;
		//case 0xED: break;
		//case 0xEE: break;
		//case 0xEF: break;
		//case 0xF0: break;
		//case 0xF1: break;
		//case 0xF2: break;
		//case 0xF3: break;
		//case 0xF4: break;
		//case 0xF5: break;
		//case 0xF6: break;
		//case 0xF7: break;
		//case 0xF8: break;
		//case 0xF9: break;
		//case 0xFA: break;
		//case 0xFB: break;
		case 0xFC: break; // Pawlov by elitegroup
		case 0xFD: break; // Ape Escape
		case 0xFE: break; // Ape Escape
		case 0xFF: break; // Pawlov by elitegroup

		default:
			DebugPrint("Unknown Command %02X", write.command);
			break;
	};
}

void PSXgpu::WriteData(u32 data) // 0x1F801810 GP0
{
	DebugPrint("[%08X] (%d) [%d]", data, gpcount, GPUMODE);

	switch(GPUMODE)
	{
	case GPUMODE_COMMAND: Command(data); break;

	case GPUMODE_DRAW_POLY3:  DrawPoly3  (data); break;
	case GPUMODE_DRAW_POLY3T: DrawPoly3T (data); break;
	case GPUMODE_DRAW_POLY3S: DrawPoly3S (data); break;
	case GPUMODE_DRAW_POLY3ST:DrawPoly3ST(data); break;
	case GPUMODE_DRAW_POLY4:  DrawPoly4  (data); break;
	case GPUMODE_DRAW_POLY4T: DrawPoly4T (data); break;
	case GPUMODE_DRAW_POLY4S: DrawPoly4S (data); break;
	case GPUMODE_DRAW_POLY4ST:DrawPoly4ST(data); break;

	case GPUMODE_DRAW_LINE:   DrawLine  (data); break;
	case GPUMODE_DRAW_LINES:  DrawLineS (data); break;
	case GPUMODE_DRAW_LINEG:  DrawLineG (data); break;
	case GPUMODE_DRAW_LINEGS: DrawLineGS(data); break;

	case GPUMODE_DRAW_RECT0:  DrawRect0  (data); break;
	case GPUMODE_DRAW_RECT0T: DrawRect0T (data); break;
	case GPUMODE_DRAW_RECT1:  DrawRect1  (data); break;
	case GPUMODE_DRAW_RECT1T: DrawRect1T (data); break;
	case GPUMODE_DRAW_RECT8:  DrawRect8  (data); break;
	case GPUMODE_DRAW_RECT8T: DrawRect8T (data); break;
	case GPUMODE_DRAW_RECT16: DrawRect16 (data); break;
	case GPUMODE_DRAW_RECT16T:DrawRect16T(data); break;

	case GPUMODE_FILL: DrawFill(data); break;

	case GPUMODE_TRANSFER_VRAM_VRAM: Transfer_VRAM_VRAM(data); break;
	case GPUMODE_TRANSFER_CPU_VRAM: Transfer_CPU_VRAM(data); break;
	case GPUMODE_TRANSFER_VRAM_CPU: Transfer_VRAM_CPU(data); break;
	
	case GPUMODE_DUMMY:
		if(gpcount >= gpsize)
		{
			GPUSTAT.READYVRAMCPU = 0;
			SetReady();
		}
		break;
	}

	gpcount++;
}

void PSXgpu::ReadDataMem(u32 * pMem, int iSize)
{
	DebugPrint("(%d)", iSize);

	for(int i = 0; i < iSize; i++) pMem[i] = ReadData();
}

void PSXgpu::WriteDataMem(u32 * pMem, int iSize)
{
	DebugPrint("(%d)", iSize);
	for(int i = 0; i < iSize; i++) WriteData(pMem[i]);
}

void PSXgpu::SetMode(u32 data)
{
	DebugPrint("[%08X]", data);
}

u32 PSXgpu::GetMode()
{
	DebugFunc();
	return 0;
}

int PSXgpu::DmaChain(u32 *base32, u32 addr)
{
	int counter = 0;
	u32 prevword[2] = {-1, -1};

	while (GPUSTAT.DMADIR == 2)
	{
		u32 *currword = &base32[addr >> 2]; // Do out of bounds check?

		if((currword[0] == prevword[0]) || (currword[0] == prevword[1])) break;
		prevword[counter&1] = currword[0];

		//printf("DmaChain: (%08X|%08X) 0[%08X]\n", currword, addr, currword[0]);
		
		u8 size = currword[0] >> 24;
		addr    = currword[0] & 0xFFFFFF;

		if(size) WriteDataMem(&currword[1], size);
		if(addr == 0xFFFFFF || ++counter > 2000000) break;
	}

	return 0;
}

void PSXgpu::SaveState(u32 &STATUS, u32 *CTRL, u8 *MEM)
{
	STATUS = GPUSTAT.GetU32();
	memcpy(MEM, VRAM.BYTE1, 1024*512*2);

	CTRL[0] = GPUMODE;
	CTRL[1] = GPUREAD;
	CTRL[2] = DISP_START;
	CTRL[3] = DISP_RANGEH;
	CTRL[4] = DISP_RANGEV;
	CTRL[5] = TEXTURE_WINDOW;
	CTRL[6] = DRAW_AREA_TL;
	CTRL[7] = DRAW_AREA_BR;
	CTRL[8] = DRAW_OFFSET;
	CTRL[9] = DC;
}

void PSXgpu::LoadState(u32 &STATUS, u32 *CTRL, u8 *MEM)
{
	GPUSTAT.SetU32(STATUS);
	memcpy(VRAM.BYTE1, MEM, 1024*512*2);

	GPUMODE        = CTRL[0];
	GPUREAD        = CTRL[1];
	DISP_START     = CTRL[2];
	DISP_RANGEH    = CTRL[3];
	DISP_RANGEV    = CTRL[4];
	TEXTURE_WINDOW = CTRL[5];
	DRAW_AREA_TL   = CTRL[6];
	DRAW_AREA_BR   = CTRL[7];
	DRAW_OFFSET    = CTRL[8];
	DC             = CTRL[9];
}

