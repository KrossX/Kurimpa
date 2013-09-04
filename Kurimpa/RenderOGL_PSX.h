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

#ifndef RENDEROGL_PSX_H
#define RENDEROGL_PSX_H
#include "Backend_OpenGL.h"

class psxProgram : public OGLProgram
{
	u32 Sampler;
	u32 DisplaySize;
	u32 DisplayOffset;

public:
	bool LoadShadersFromFile(const char *vpath, const char *fpath, const char* defs);
	bool LoadShadersFromBuff(const char *vbuff, const char *fbuff, const char* defs);
	void SetDisplaySize(int ox, int oy, int width, int height);
	void SetDisplayOffset(int offx1, int offy1, int offx2, int offy2);

	psxProgram() { Sampler = DisplaySize = DisplayOffset = 0; }
};

class RenderOGL_PSX : public Backend_OpenGL
{
	bool linearfilter;
	bool usevsync;
	bool fullscreen;
	bool show_vram;

	enum
	{
		PROG_FB16,
		PROG_FB24,
		PROG_VRAM,
		PROG_SIZE
	};

	psxProgram Prog[PROG_SIZE];

	struct
	{
		u8 *vram;
		int width, height;
		int ox, oy;
		int offx1, offx2;
		int offy1, offy2;
	} psx;

	struct
	{
		u32 tVRAM; // Main VRAM texture
		u32 background_vb, background_uv; // Background Quad/UV coords
		u32 background_va; // Backgrond vertex array
	} gl;

	bool CreateVRAMtexture(u32 *tex);

	void ToggleVRAM();
	void ToggleVsync();
	void ToggleFiltering();
	void ToggleFullscreen();
	void WindowResize();

	void TakeVRAMshot();

public:
	void SetDisplayOffset(int ox, int oy);
	void SetDisplayMode(int width, int height);
	void SetPSXoffset(int x1, int x2, int y1, int y2);

	void WndProc(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	void Present(bool is24bpp, bool disabled);
	bool Init(HWND hWin, u8 *psxvram);
	void Shutdown();

	RenderOGL_PSX();
};


#endif