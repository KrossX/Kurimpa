////////////////////////////////////////////////////////////////////////
// PPDK developer must change libraryName field and can change revision and build
////////////////////////////////////////////////////////////////////////

#define VERSION_MAJOR 1
#define VERSION_MINOR 2

static char lib_name[]   = "Kurimpa Video Plugin";
static char lib_author[] = "KrossX";

////////////////////////////////////////////////////////////////////////
// stuff to make this a true PDK module
////////////////////////////////////////////////////////////////////////

char * CALLBACK PSEgetLibName(void)
{
	wrap_PSEgetLibName();
	return lib_name;
}

unsigned long CALLBACK PSEgetLibType(void)
{
	wrap_PSEgetLibType();
	return PSE_LT_GPU;
}

unsigned long CALLBACK PSEgetLibVersion(void)
{
	wrap_PSEgetLibVersion();
	return PLUGIN_VERSION << 16 | VERSION_MAJOR << 8 | VERSION_MINOR;
}

////////////////////////////////////////////////////////////////////////
// Init/shutdown, will be called just once on emu start/close
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUinit(void)
{
	wrap_GPUinit();
	
	gpu.vram   = VirtualAlloc(NULL, PSX_VRAM_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	gpu.status_reg = 0x14802000; // HACK: Boot state
	return gpu.vram ? PSE_GPU_ERR_SUCCESS : PSE_GPU_ERR_INIT;
}

long CALLBACK GPUshutdown(void)
{
	wrap_GPUshutdown();
	
	VirtualFree(gpu.vram, 0, MEM_RELEASE);
	return PSE_GPU_ERR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// Snapshot func, save some snap into 
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUmakeSnapshot(void)
{
	wrap_GPUmakeSnapshot();
}

////////////////////////////////////////////////////////////////////////
// Open/close will be called when a games starts/stops
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUopen(void *handle)
{
	wrap_GPUopen(window_other);
	return video_open(handle);
}

long CALLBACK GPUclose(void)
{
	wrap_GPUclose();
	return video_close();
}

////////////////////////////////////////////////////////////////////////
// UpdateLace will be called on every vsync
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUupdateLace(void)
{
	wrap_GPUupdateLace();
	
	if ((gpu.status_reg >> 22) & 1)
		gpu.status_reg ^= 0x80000000; // HACK: odd/even crazyness
	else
		gpu.status_reg &= 0x7FFFFFFF;
	
	video_update();
}

////////////////////////////////////////////////////////////////////////
// process read request from GPU status register
////////////////////////////////////////////////////////////////////////

unsigned long CALLBACK GPUreadStatus(void)
{
	u32 other = wrap_GPUreadStatus() & 0x7FFFFFFF;
	
	//wsprintf(message[message_idx++], "GPUSTAT %08X | %08X\n", gpu.status_reg&0x7FFFFFFF, other);
	
	// if(other != gpu.status_reg)
		// save_textlog();
	
	return gpu.status_reg;
}

////////////////////////////////////////////////////////////////////////
// processes data send to GPU status register
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUwriteStatus(unsigned long data)
{
	wrap_GPUwriteStatus(data);
	gpu_status(data);
}

////////////////////////////////////////////////////////////////////////
// core read from vram
////////////////////////////////////////////////////////////////////////

unsigned long CALLBACK GPUreadData(void)
{
	wrap_GPUreadData();
	
	if (gpu.mode == GPUMODE_COPY_VRAM_CPU)
	{
		gpu_data(0);
	}

	return gpu.return_data;
}

void CALLBACK GPUreadDataMem(unsigned long *mem, int len)
{
	void *mem2 = mem;
	wrap_GPUreadDataMem(mem2, len);
	
	if (len > 0) while (len--)
	{
		if (gpu.mode == GPUMODE_COPY_VRAM_CPU)
		{
			gpu_data(0);
		}
		
		*mem++ = gpu.return_data;
	}
}

////////////////////////////////////////////////////////////////////////
// core write to vram
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUwriteData(unsigned long data)
{
	wrap_GPUwriteData(data);
	gpu_data(data);
}

void CALLBACK GPUwriteDataMem(unsigned long *mem, int len)
{
	void *mem2 = mem;
	
	wrap_GPUwriteDataMem(mem2, len);

	if (len > 0) while (len--)
	{
		gpu_data(*mem++);
	}
}

////////////////////////////////////////////////////////////////////////
// setting/getting the transfer mode (this functions are obsolte)
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUsetMode(unsigned long gdata)
{
	wrap_GPUsetMode(gdata);
}

long CALLBACK GPUgetMode(void)
{
	wrap_GPUgetMode();
	return 0;
}

////////////////////////////////////////////////////////////////////////
// dma chain, process gpu commands
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUdmaChain(unsigned long * baseAddrL, unsigned long addr)
{
	u32 cnt = 0;
	u32 ptr = (u32)(addr & 0xFFFFFF);

	u32 *mem = (u32*)baseAddrL;
	u32 *cur;
	
	int len;
	
	void *baseadr = baseAddrL;
	wrap_GPUdmaChain(baseadr, addr);

/*
	while (ptr != 0xFFFFFF)
	{
		cur = &mem[ptr >> 2];
		ptr = cur[0] & 0xFFFFFF;

		len = cur[0] >> 24;
		if (len) GPUwriteDataMem((void*)&cur[1], len);
	}
*/

	while (ptr != 0xFFFFFF)
	{
		cur = &mem[ptr >> 2];
		ptr = cur[0] & 0x1FFFFC;
		
		if(ptr == addr || cnt > 0x1FFFFF)
			break;

		len = cur[0] >> 24;
		
		cnt += 4 * len + 4;
		
		
		if (len) GPUwriteDataMem((void*)&cur[1], len);
	}

	return PSE_GPU_ERR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// call config dlg
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUconfigure(void)
{
	wrap_GPUconfigure();
	
	return PSE_GPU_ERR_SUCCESS;
}

////////////////////////////////////////////////////////////////////////
// show about dlg
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUabout(void)
{
	wrap_GPUabout();
}

////////////////////////////////////////////////////////////////////////
// test... well, we are ever fine ;)
////////////////////////////////////////////////////////////////////////

long CALLBACK GPUtest(void)
{
	wrap_GPUtest();
	
	// if test fails this function should return negative value for error (unable to continue)
	// and positive value for warning (can continue but output might be crappy)
	return PSE_GPU_ERR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////
// special debug function, only available in Pete's Soft GPU
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUdisplayText(char *text)
{
	wrap_GPUdisplayText(text);
}

////////////////////////////////////////////////////////////////////////
// special info display function, only available in Pete's GPUs right now
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUdisplayFlags(unsigned long flags)
{
	// currently supported flags: 
	// bit 0 -> Analog pad mode activated
	// bit 1 -> PSX mouse mode activated
	// display this info in the gpu menu/fps display
	wrap_GPUdisplayFlags(flags);
}

////////////////////////////////////////////////////////////////////////
// Freeze
////////////////////////////////////////////////////////////////////////

struct GPUFreeze
{
	unsigned long version;              // should be always 1 for now (set by main emu)
	unsigned long status;               // current gpu status
	unsigned long control[256];         // latest control register values
	unsigned char vram[1024 * 512 * 2]; // current VRam image
};

////////////////////////////////////////////////////////////////////////

long CALLBACK GPUfreeze(unsigned long mode, struct GPUFreeze *freeze)
{
	wrap_GPUfreeze(mode, freeze);
	
	switch (mode)
	{
	case 0: // set data
		break;
	case 1: // get data
		break;
	case 2: // info
		break;
	}

	return 1;
}


////////////////////////////////////////////////////////////////////////
// ScreenPic
////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////
// the main emu allocs 128x96x3 bytes, and passes a ptr
// to it in pMem... the plugin has to fill it with
// 8-8-8 bit BGR screen data (Win 24 bit BMP format 
// without header). 
// Beware: the func can be called at any time,
// so you have to use the frontbuffer to get a fully
// rendered picture

void CALLBACK GPUgetScreenPic(unsigned char *mem)
{
	wrap_GPUshowScreenPic(mem);
}

////////////////////////////////////////////////////////////////////////
// func will be called with 128x96x3 BGR data.
// the plugin has to store the data and display
// it in the upper right corner.
// If the func is called with a NULL ptr, you can
// release your picture data and stop displaying
// the screen pic

void CALLBACK GPUshowScreenPic(unsigned char *mem)
{
	wrap_GPUgetScreenPic(mem);
}

////////////////////////////////////////////////////////////////////////
// gun cursor func: player=0-7, x=0-511, y=0-255
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUcursor(int iPlayer, int x, int y)
{

}

////////////////////////////////////////////////////////////////////////
// Allows ePSXe to change framelimit settings
////////////////////////////////////////////////////////////////////////

void CALLBACK GPUsetframelimit(unsigned long option)
{

}


// UNKNOWN
void CALLBACK GPUsetSpeed(float newSpeed)
{

}

void CALLBACK GPUsetfix(int dwFixBits)
{

}

void CALLBACK GPUvBlank(int val)
{

}

void CALLBACK GPUhSync(int val)
{

}

void CALLBACK GPUvisualVibration(int iSmall, int iBig)
{

}