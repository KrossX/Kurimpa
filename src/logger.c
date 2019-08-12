char message[256][256];
u8  message_idx;

u8 last_cmd;

static int logger_idx;
static int bitmap_fileid;
static int logger_enabled;

#if 1
	#define LOG_PRINTF wsprintf
#else
	#define LOG_PRINTF
#endif

#define LOG_MAX_TXT (10)
#define LOG_MAX_BMP (10)

void save_textlog(void)
{
	char namebuf[128];
	HANDLE file;
	DWORD written;
	int i;
	
	if(logger_idx > LOG_MAX_TXT) return;
	
	CreateDirectory("logtxt", NULL);
	
	wsprintf(namebuf, "logtxt/%04d.txt", logger_idx++);
	file = CreateFile(namebuf, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
	if(file == INVALID_HANDLE_VALUE) return;
	
	for(i = 0; i < 256; i++)
	{
		u8 idx = message_idx + i;
		char *line = message[idx];
		
		if(line[0])
		{
			WriteFile(file, line, strlen(line), &written, NULL);
		}
	}
	
	CloseHandle(file);
}

u32 bmp32[512 * 1024];

void convert_to_bmp32(int x, int y, int w, int h, u16 *buffer)
{
	int i, j;
	
	for(j = 0; j < h; j++)
	{
		u16 *line16 = &buffer[(y + j) * 1024];
		u32 *line32 = &bmp32[(y + j) * 1024];
		
		for(i = 0; i < w; i++)
		{
			u16 col16 = line16[x + i];
			u32 col32  = ((col16 & 0x1F) * 0xFF) / 0x1F;
			    col32 |= ((((col16 >> 5) & 0x1F) * 0xFF) / 0x1F) << 8;
				col32 |= ((((col16 >> 10) & 0x1F) * 0xFF) / 0x1F) << 16;
				col32 |= ((col16 >> 15) * 0xFF) << 24;
			
			line32[x + i] = col32;
		}
	}
}

void save_bitmap(int x, int y, int w, int h, u16 *buffer)
{
	BITMAPFILEHEADER bmfh;
	BITMAPINFOHEADER info;

	HANDLE file;
	DWORD written;
	int j, wpad;
	
	CreateDirectory("logbmp", NULL);
	
	while(bitmap_fileid < LOG_MAX_BMP)
	{
		char namebuf[128];
		wsprintf(namebuf, "logbmp/%04d.bmp", bitmap_fileid++);
		file = CreateFile(namebuf, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		if(file != INVALID_HANDLE_VALUE) break;
	}

	if(file == INVALID_HANDLE_VALUE) return;

	wpad = (w+3)&~3; // padding
	
	bmfh.bfType      = 0x4D42; // must be "BM"
	bmfh.bfSize      = sizeof(bmfh) + sizeof(info) + wpad * h * 4; //size of file in bytes
	bmfh.bfReserved1 = 0;
	bmfh.bfReserved2 = 0;
	bmfh.bfOffBits   = sizeof(bmfh) + sizeof(info); // Offset to data
	
	info.biSize          = sizeof(info); 
	info.biWidth         = w;
	info.biHeight        = h;
	info.biPlanes        = 1; 
	info.biBitCount      = 32;
	info.biCompression   = BI_RGB; 
	info.biSizeImage     = 0; 
	info.biXPelsPerMeter = 2835; 
	info.biYPelsPerMeter = 2835; 
	info.biClrUsed       = 0;
	info.biClrImportant  = 0;
	
	WriteFile(file, &bmfh, sizeof(bmfh), &written, NULL);
	WriteFile(file, &info, sizeof(info), &written, NULL);
	
	convert_to_bmp32(x, y, w, h, buffer);
	
	for(j = h-1; j >= 0; j--)
	{
		WriteFile(file, &bmp32[(y + j) * 1024 + x], wpad * 4, &written, NULL);
	}
	
	CloseHandle(file);
}

#define LOG_CHECK_WIDTH (32)
#define LOG_CHECK_HEIGHT (32)

void compare_mem(u16 *mem1, u16 *mem2)
{
	int i, j, differ = 0;
	
	if(!logger_enabled) return;
	if(bitmap_fileid >= LOG_MAX_BMP) return;
	
	for(j = 0; j < LOG_CHECK_HEIGHT; j++)
	{
		u16 *line1 = &mem1[j * 1024];
		u16 *line2 = &mem2[j * 1024];
		
		for(i = 0; i < LOG_CHECK_WIDTH; i++)
		{
			if(line1[i] != line2[i])
			{
				differ = 1;
				break;
			}
		}
		
		if(differ) break;
	}
	
	if(differ)
	{
		logger_idx = bitmap_fileid;
		save_textlog();
		save_bitmap(0, 0, LOG_CHECK_WIDTH, LOG_CHECK_HEIGHT, mem1);
		save_bitmap(0, 0, LOG_CHECK_WIDTH, LOG_CHECK_HEIGHT, mem2);
	}
}