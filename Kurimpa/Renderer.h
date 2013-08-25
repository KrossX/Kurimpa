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

struct Renderer
{
	s32 offset_x1, offset_y1;
	s32 offset_x2, offset_y2;
	s32 origin_x, origin_y;
	s32 width, height;

	virtual bool Init(HWND hWin) = 0;
	virtual void Close() = 0;
	virtual void Present(bool is24bpp, bool disabled) = 0;
	virtual void UpdateTitle(const char* title) = 0;
};

struct OGL_ShaderProgram
{
	u32 ProgramID, SamplerID;
	u32 u_WindowSize, u_DisplaySize;
	u32 u_DisplayOffset;
};

class RenderOGL : public Renderer
{
	enum
	{
		PROG_FB16,
		PROG_FB24,
		PROG_VRAM,
		PROG_SIZE
	};

	OGL_ShaderProgram Prog[PROG_SIZE];

	u32 tVRAM_ID, tDisplay_ID;
	u32 VertexArrayID;

	u32 vb_background, uv_background;

	inline void DrawBackground();
	inline void PrepareQuad();

public:
	bool Init(HWND hWin);
	void Close();
	void Present(bool is24bpp, bool disabled);
	void UpdateTitle(const char* title);
};