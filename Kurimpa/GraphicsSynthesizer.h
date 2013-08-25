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

class GraphicsSynthesizer
{
	HINSTANCE hInstance;
	LPCWSTR applicationName;
	HWND hWnd;

public:
	WNDPROC emuWndProc;

	int Init();
	void Shutdown();
	
	int Open(void** dsp, char* title, int mt);
	int Open2(void** dsp, u32 flags);
	void Close();
	
	void Vsync(int field);

	void SetBaseMem(u8* mem);
	void Reset();
	
	void WriteCSR(u32 csr);
	void ReadFIFO(u64* mem);
	void ReadFIFO2(u64* mem, u32 qwc);

	void GIF_SoftReset(u32 mask);
	void GIF_Transfer(const u8* mem, u32 size);
	void GIF_Transfer1(u8* mem, u32 addr);
	void GIF_Transfer2(u8* mem, u32 size);
	void GIF_Transfer3(u8* mem, u32 size);

	void IRQ_Callback(void (*irq)());
	
	void GetLastTag(u32* tag);
	void GetTitleInfo2(char* dest, size_t length);
	
	void SetFrameSkip(int frameskip);
	void SetVsync(int enabled);
	void SetExclusive(int enabled);
	void SetFrameLimit(int limit);
};