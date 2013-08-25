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
#include "GraphicsSynthesizer.h"

#include <math.h>
#include <stdlib.h>

void Rasterizer::Open(HWND _hViewport)
{
	hViewport = _hViewport;
	GetClientRect(hViewport,&rViewport);
}

void Rasterizer::Point()
{
	GS_Registers::SInternal::SVERTEX * v0 = VertexQueue.V0.Vertex;
	
	u16 winX = u16(v0->XYZ.X - Env.Context->XYOFFSET.OFX);
	u16 winY = u16(v0->XYZ.Y - Env.Context->XYOFFSET.OFY);

	PIXEL pixy;
	pixy.SetColor(v0->RGBA.R, v0->RGBA.G, v0->RGBA.B, v0->RGBA.A);
	pixy.SetCoord(winX, winY, v0->XYZ.Z);
};

void Rasterizer::Line()
{
	//printf("Kurimpa->DrawLine!\n");
};

void Rasterizer::Triangle()
{
	//printf("Kurimpa->DrawTriangle!\n");
};

void Rasterizer::Sprite()
{	
	//printf("Kurimpa->DrawSprite!\n");
};

void Rasterizer::PixelPipeline(PIXEL pix)
{
	if(pix.COORD.X < Env.Context->SCISSOR.SCAX0 || pix.COORD.X > Env.Context->SCISSOR.SCAX1) return;
	if(pix.COORD.Y < Env.Context->SCISSOR.SCAY0 || pix.COORD.Y > Env.Context->SCISSOR.SCAY1) return;
};

void Rasterizer::Finish()
{
	//printf("FINISH!\n");
};

void Rasterizer::Vsync(int field)
{				

};

