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
#include "Main.h"
#include "PSXgpu_Enums.h"
#include "PSXgpu.h"
#include "GraphicsSynthesizer.h"

GraphicsSynthesizer * GS = NULL; PSXgpu * PSXGPU = NULL;
////////////////////////////////////////////////////////////////////////
// PPDK developer must change libraryName field and can change revision and build
////////////////////////////////////////////////////////////////////////

const  u8 version  = 1;    // do not touch - library for PSEmu 1.x
const  u8 revision = 0;
const  u8 build    = 1;

const u32 versionPS1 = (emupro::PLUGIN_VERSION << 16) | (revision << 8) | build;
const u32 versionPS2 = (0x06 << 16) | (revision << 8) | build;

static char *libraryName      = "Kurimpa Video Plugin";
static char *PluginAuthor     = "KrossX";

////////////////////////////////////////////////////////////////////////
// stuff to make this a true PDK module
////////////////////////////////////////////////////////////////////////

char * CALLBACK PSEgetLibName(void)
{
	//printf("Kurimpa -> PSEgetLibName\n");
	return libraryName;
}

u32 CALLBACK PSEgetLibType(void)
{
	//printf("Kurimpa -> PSEgetLibType\n");
	return  emupro::LT_GPU;
}

u32 CALLBACK PSEgetLibVersion(void)
{
	//printf("Kurimpa -> PSEgetLibVersion\n");
	return versionPS1;
}

////////////////////////////////////////////////////////////////////////
// PCSX2 stuff
////////////////////////////////////////////////////////////////////////

u32 CALLBACK PS2EgetLibType()
{
	//printf("Kurimpa -> PS2EgetLibType\n");	
	return 0x01;
}

const char* CALLBACK PS2EgetLibName()
{
	//printf("Kurimpa -> PS2EgetLibName\n");	
	return libraryName;
}

u32 CALLBACK PS2EgetLibVersion2(u32 type)
{	
	//printf("Kurimpa -> PS2EgetLibVersion2: %X\n", type);	
	return versionPS2; 	
}

u32 CALLBACK PS2EgetCpuPlatform()
{
	//printf("Kurimpa -> PS2EgetCpuPlatform\n");
	return 0x01; // x86-32
}

void CALLBACK PS2EsetEmuVersion(const char* emuId, u32 version)
{
	// ??
}

////////////////////////////////////////////////////////////////////////
// Init/shutdown, will be called just once on emu start/close
////////////////////////////////////////////////////////////////////////
 
int CALLBACK GPUinit()                                // GPU INIT
{		
	PSXGPU = new PSXgpu();
	if(PSXGPU) return PSXGPU->Init();
	else return emupro::ERR_FATAL;
}

int CALLBACK GSinit()
{
	GS = new GraphicsSynthesizer();
	if(GS) return GS->Init();
	else return emupro::ERR_FATAL;
}

int CALLBACK GPUshutdown()
{	
	return PSXGPU->Shutdown();
}

void CALLBACK GSshutdown()
{
	GS->Shutdown();
}

////////////////////////////////////////////////////////////////////////
// Snapshot func, save some snap into 
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUmakeSnapshot(void)
{
	PSXGPU->TakeSnapshot();
}

////////////////////////////////////////////////////////////////////////
// Open/close will be called when a games starts/stops
////////////////////////////////////////////////////////////////////////

int CALLBACK GPUopen(HWND hwndGPU)
{
	return PSXGPU->Open(hwndGPU);
}

int CALLBACK GPUclose()
{
	return PSXGPU->Close();
}

void CALLBACK GSclose()
{
	GS->Close();
}

int CALLBACK GSopen2(void** dsp, u32 flags)
{
	return GS->Open2(dsp, flags);
}

int CALLBACK GSopen(void** dsp, char* title, int mt)
{
	return GS->Open(dsp, title, mt);
}


////////////////////////////////////////////////////////////////////////
// UpdateLace will be called on every vsync
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUupdateLace(void)
{
	PSXGPU->LaceUpdate();
}

void CALLBACK GSvsync(int field)
{
	GS->Vsync(field);
}

////////////////////////////////////////////////////////////////////////
// process read request from GPU status register
////////////////////////////////////////////////////////////////////////

u32 CALLBACK GPUreadStatus(void)
{
	return PSXGPU->ReadStatus();
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU status register
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUwriteStatus(u32 gdata)
{
	PSXGPU->WriteStatus(gdata);
}

////////////////////////////////////////////////////////////////////////
// core read from vram
////////////////////////////////////////////////////////////////////////

u32 CALLBACK GPUreadData(void)
{	
	return PSXGPU->ReadData();
}

// new function, used by ePSXe, for example, to read a whole chunk of data

void CALLBACK GPUreadDataMem(u32 * pMem, int iSize)
{
	PSXGPU->ReadDataMem(pMem, iSize);
}

////////////////////////////////////////////////////////////////////////
// core write to vram
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUwriteData(u32 gdata)
{
	PSXGPU->WriteData(gdata);
}

// new function, used by ePSXe, for example, to write a whole chunk of data

void CALLBACK GPUwriteDataMem(u32 * pMem, int iSize)
{
	PSXGPU->WriteDataMem(pMem, iSize);
}

////////////////////////////////////////////////////////////////////////
// setting/getting the transfer mode (this functions are obsolte)
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUsetMode(u32 gdata)
{
	PSXGPU->SetMode(gdata);
}

// this function will be removed soon
u32 CALLBACK GPUgetMode(void)
{
	return PSXGPU->GetMode();
}

////////////////////////////////////////////////////////////////////////
// dma chain, process gpu commands
////////////////////////////////////////////////////////////////////////

int CALLBACK GPUdmaChain(u32 * baseAddrL, u32 addr)
{
	return PSXGPU->DmaChain(baseAddrL, addr);
}

////////////////////////////////////////////////////////////////////////
// call config dlg
////////////////////////////////////////////////////////////////////////

int CALLBACK GPUconfigure(void)
{	
	printf("Kurimpa -> GPUConfigure...\n");
	return emupro::gpu::ERR_SUCCESS;
}

void CALLBACK  GSconfigure()
{
	printf("Kurimpa -> GSconfigure\n");
}

////////////////////////////////////////////////////////////////////////
// show about dlg
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUabout(void)                           // ABOUT?
{
	printf("Kurimpa -> GPUAbout...\n");
}

void CALLBACK GSabout()
{
	printf("Kurimpa -> GSabout\n");
}

////////////////////////////////////////////////////////////////////////
// test... well, we are ever fine ;)
////////////////////////////////////////////////////////////////////////

int CALLBACK GPUtest(void)
{
	// if test fails this function should return negative value for error (unable to continue)
	// and positive value for warning (can continue but output might be crappy)
	printf("Kurimpa -> GPUTest\n");
	return emupro::gpu::ERR_SUCCESS;
}

int CALLBACK GStest()
{
	printf("Kurimpa -> GStest\n");
	return 0;
}

////////////////////////////////////////////////////////////////////////
// special debug function, only available in Pete's Soft GPU
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUdisplayText(char * pText)
{
	printf("Kurimpa -> GPUDisplayText [%s]\n", pText? pText : "");
}

////////////////////////////////////////////////////////////////////////
// special info display function, only available in Pete's GPUs right now
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUdisplayFlags(u32 dwFlags)
{
	// currently supported flags: 
	// bit 0 -> Analog pad mode activated
	// bit 1 -> PSX mouse mode activated
	// display this info in the gpu menu/fps display
	printf("Kurimpa -> GPUDisplayFlags [%X]\n", dwFlags);
}

////////////////////////////////////////////////////////////////////////
// Freeze
////////////////////////////////////////////////////////////////////////

typedef struct
{
 u32 ulFreezeVersion;      // should be always 1 for now (set by main emu)
 u32 ulStatus;             // current gpu status
 u32 ulControl[256];       // latest control register values
 u8 psxVRam[1024*512*2];  // current VRam image
} GPUFreeze_t;

////////////////////////////////////////////////////////////////////////

int CALLBACK GPUfreeze(u32 mode,GPUFreeze_t * pF)
{
	switch(mode)
	{
	case 0: // LOAD
		if(!pF || pF->ulFreezeVersion != 1) return 0;
		PSXGPU->LoadState(pF->ulStatus, pF->ulControl, pF->psxVRam);
		break;

	case 1: // SAVE
		if(!pF || pF->ulFreezeVersion != 1) return 0;
		PSXGPU->SaveState(pF->ulStatus, pF->ulControl, pF->psxVRam);
		break;

	case 2: // QUERY
		// Use it to get save slot?
		break;
	};

	return 1;
}

////////////////////////////////////////////////////////////////////////
// Who knos what this are for
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUclearDynarec(void (CALLBACK *fnc)()) // ??? 
{
	printf("Kurimpa -> GPUclearDynarec\n");
}

////////////////////////////////////////////////////////////////////////
// the main emu allocs 128x96x3 bytes, and passes a ptr
// to it in pMem... the plugin has to fill it with
// 8-8-8 bit BGR screen data (Win 24 bit BMP format 
// without header). 
// Beware: the func can be called at any time,
// so you have to use the frontbuffer to get a fully
// rendered picture


void CALLBACK GPUgetScreenPic(u8* mem)
{
	printf("Kurimpa -> GPUgetScreenPic [%X]\n", mem);
}

////////////////////////////////////////////////////////////////////////
// func will be called with 128x96x3 BGR data.
// the plugin has to store the data and display
// it in the upper right corner.
// If the func is called with a NULL ptr, you can
// release your picture data and stop displaying
// the screen pic

void CALLBACK GPUshowScreenPic(u8* mem)
{
	printf("Kurimpa -> GPUgetScreenPic [%X]\n", mem);
}

////////////////////////////////////////////////////////////////////////
// gun cursor func: player=0-7, x=0-511, y=0-255
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUcursor(int player, int x, int y)
{
	printf("Kurimpa -> GPUcursor [%d | %d, %d]\n", player, x, y);
}

////////////////////////////////////////////////////////////////////////
// Who knos what this are for - PCSX2 style
////////////////////////////////////////////////////////////////////////

void CALLBACK  GSsetSettingsDir(const char* dir)
{
	printf("Kurimpa -> SettingsDir [%s]\n", dir);
}

void CALLBACK  GSsetBaseMem(u8* mem) { GS->SetBaseMem(mem); }
void CALLBACK  GSreset() { GS->Reset(); }
void CALLBACK  GSgifSoftReset(u32 mask) { GS->GIF_SoftReset(mask); }
void CALLBACK  GSwriteCSR(u32 csr) { GS->WriteCSR(csr); }
void CALLBACK  GSreadFIFO(u64* mem) { GS->ReadFIFO(mem); }
void CALLBACK  GSreadFIFO2(u64* mem, u32 qwc) { GS->ReadFIFO2(mem, qwc); }
void CALLBACK  GSgifTransfer(const u8* mem, u32 size) { GS->GIF_Transfer(mem, size); }
void CALLBACK  GSgifTransfer1(u8* mem, u32 addr) { GS->GIF_Transfer1(mem, addr); }
void CALLBACK  GSgifTransfer2(u8* mem, u32 size) { GS->GIF_Transfer2(mem, size); }
void CALLBACK  GSgifTransfer3(u8* mem, u32 size) { GS->GIF_Transfer3(mem, size); }

u32 CALLBACK GSmakeSnapshot(char* path)
{
	printf("Kurimpa -> GSmakeSnapshot\n");
	return 0;
}

struct KeyEventData {u32 key, type;};

void CALLBACK  GSkeyEvent(KeyEventData* e)
{
	printf("Kurimpa -> GSkeyEvent\n");
}

struct FreezeData {int size; u32* data;};

int CALLBACK GSfreeze(int mode, FreezeData* data)
{
	printf("Kurimpa -> GSfreeze\n");
	return 0;
}

int CALLBACK GSsetupRecording(int start, void* data)
{
	printf("Kurimpa -> GSsetupRecording\n");
	return 0;
}

void CALLBACK  GSsetGameCRC(u32 crc, int options)
{
	printf("Kurimpa -> GSsetGameCRC\n");
}

void CALLBACK  GSirqCallback(void (*irq)()) { GS->IRQ_Callback(irq); }
void CALLBACK  GSgetLastTag(u32* tag) { GS->GetLastTag(tag); }
void CALLBACK  GSgetTitleInfo2(char* dest, size_t length) { GS->GetTitleInfo2(dest, length); }
void CALLBACK  GSsetFrameSkip(int frameskip) { GS->SetFrameSkip(frameskip); }
void CALLBACK  GSsetVsync(int enabled) { GS->SetVsync(enabled); }
void CALLBACK  GSsetExclusive(int enabled) { GS->SetExclusive(enabled); }
void CALLBACK  GSsetFrameLimit(int limit) { GS->SetFrameLimit(limit); }

void CALLBACK  GSReplay(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	printf("Kurimpa -> GSReplay\n");
}

/*
void CALLBACK  GSBenchmark(HWND hwnd, HINSTANCE hinst, LPSTR lpszCmdLine, int nCmdShow)
{
	printf("Kurimpa -> GSBenchmark\n");
}
*/