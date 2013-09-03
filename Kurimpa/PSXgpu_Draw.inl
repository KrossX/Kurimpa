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



inline void PSXgpu::DrawStart(u32 data)
{
	//SetReadyCMD(0);
	//SetReadyDMA(0);
	vertex[0].c = data & 0xFFFFFF;
	DC = (data >> 24) & 0xFF;
}

void PSXgpu::DrawPoly3(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].SetV(data); break;
	case 3: vertex[2].SetV(data);

		raster->UpdateDC(DC);
		raster->RasterPoly3(RENDER_FLAT);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly3T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3T;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].SetV(data); break;
	case 4: vertex[1].SetT(data);
		GPUSTAT.SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 5: vertex[2].SetV(data); break;
	case 6: vertex[2].SetT(data);

		raster->UpdateDC(DC);
		raster->RasterPoly3(RENDER_TEXTURED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly3S(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3S;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;
	case 3: vertex[1].SetV(data); break;
	case 4: vertex[2].c = data; break;
	case 5: vertex[2].SetV(data);

		raster->UpdateDC(DC);
		raster->RasterPoly3(RENDER_SHADED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly3ST(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY3ST;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].c = data; break;
	case 4: vertex[1].SetV(data); break;
	case 5: vertex[1].SetT(data);
		GPUSTAT.SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 6: vertex[2].c = data; break;
	case 7: vertex[2].SetV(data); break;
	case 8: vertex[2].SetT(data);

		raster->UpdateDC(DC);
		raster->RasterPoly3(RENDER_SHADED_TEXTURED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].SetV(data); break;
	case 3: vertex[2].SetV(data); break;
	case 4: vertex[3].SetV(data);

		raster->UpdateDC(DC);
		raster->RasterPoly4(RENDER_FLAT);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4T;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].SetV(data); break;
	case 4: vertex[1].SetT(data);
		GPUSTAT.SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 5: vertex[2].SetV(data); break;
	case 6: vertex[2].SetT(data); break;
	case 7: vertex[3].SetV(data); break;
	case 8: vertex[3].SetT(data);

		raster->UpdateDC(DC);
		raster->RasterPoly4(RENDER_TEXTURED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4S(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4S;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;
	case 3: vertex[1].SetV(data); break;
	case 4: vertex[2].c = data; break;
	case 5: vertex[2].SetV(data); break;
	case 6: vertex[3].c = data; break;
	case 7: vertex[3].SetV(data);

		raster->UpdateDC(DC);
		raster->RasterPoly4(RENDER_SHADED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawPoly4ST(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_POLY4ST;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3: vertex[1].c = data; break;
	case 4: vertex[1].SetV(data); break;
	case 5: vertex[1].SetT(data);
		GPUSTAT.SetTEXPAGE(vertex[1].tex);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 6: vertex[2].c = data; break;
	case 7: vertex[2].SetV(data); break;
	case 8: vertex[2].SetT(data); break;
	case 9: vertex[3].c = data; break;
	case 10:vertex[3].SetV(data); break;
	case 11:vertex[3].SetT(data);

		raster->UpdateDC(DC);
		raster->RasterPoly4(RENDER_SHADED_TEXTURED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawLine(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINE;
		break;

	case 1: vertex[0].SetV(data);
		break;

	case 2:
		vertex[1].SetV(data);

		raster->UpdateDC(DC);
		raster->RasterLine(RENDER_FLAT);
		SetReady();
		break;
	}
}

void PSXgpu::DrawLineS(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINES;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: 
		vertex[1].SetV(data);
		raster->UpdateDC(DC);
		raster->RasterLine(RENDER_FLAT);
		break;

	default:
		if(data == 0x55555555) SetReady();
		else
		{
			u8 vx = (gpcount % 2) ? 0 : 1; 
			vertex[vx].SetV(data);
			raster->RasterLine(RENDER_FLAT);
		}
		break;
	}
}

void PSXgpu::DrawLineG(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINEG;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;
	case 3:
		vertex[1].SetV(data);
		raster->UpdateDC(DC);
		raster->RasterLine(RENDER_SHADED);
		SetReady();
		break;
	}
}

void PSXgpu::DrawLineGS(u32 data)
{
	static u32 line = 0;

	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_LINEGS;
		line = 0;
		break;

	case 1: vertex[0].SetV(data); break;
	case 2: vertex[1].c = data; break;

	case 3:
		vertex[1].SetV(data);
		raster->UpdateDC(DC);
		raster->RasterLine(RENDER_SHADED);
		break;

	default:
		if(data == 0x55555555) SetReady();
		else
		{
			if(line % 2)
			{
				vertex[2].SetV(data);

				vertex[0] = vertex[1];
				vertex[1] = vertex[2];

				raster->RasterLine(RENDER_SHADED);
			}
			else
			{
				vertex[2].c = data;
			}

			line++;
		}
		break;
	}
}

void PSXgpu::DrawRect0(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT0;
		break;

	case 1:
		vertex[0].SetV(data);
		break;

	case 2:
		vertex[1].SetV(data); // Size
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_FLAT, 0);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect0T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT0T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		break;

	case 3:
		vertex[1].SetV(data); // Size
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_TEXTURED, 0);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect1(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT1;
		break;

	case 1:
		vertex[0].SetV(data);
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_FLAT, 1);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect1T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT1T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_TEXTURED, 1);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect8(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT8;
		break;

	case 1:
		vertex[0].SetV(data);
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_FLAT, 2);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect8T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT8T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_TEXTURED, 2);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect16(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT16;
		break;

	case 1:
		vertex[0].SetV(data);
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_FLAT, 3);
		SetReady();
		break;
	}
}

void PSXgpu::DrawRect16T(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DrawStart(data);
		GPUMODE = GPUMODE_DRAW_RECT16T;
		break;

	case 1:
		vertex[0].SetV(data);
		TW.SetPAGE(VRAM.HALF2, GPUSTAT);
		break;

	case 2:
		vertex[0].SetT(data);
		TW.SetCLUT(VRAM.HALF2, vertex[0].tex);
		raster->UpdateDC(DC);
		raster->RasterRect(RENDER_TEXTURED, 3);
		SetReady();
		break;
	}
}


void PSXgpu::DrawFill(u32 data)
{
	switch(gpcount)
	{
	case 0:
		vertex[0].c = data & 0xFFFFFF;
		gpsize = 2;
		GPUMODE = GPUMODE_FILL;
		break;

	case 1:
		TR.start.U32 = data;
		TR.start.U16[0] &= 0x3F0;
		TR.start.U16[1] &= 0x1FF;
		break;

	case 2:
		{
			TR.size.U32 = data;

			if(TR.size.U16[0] && TR.size.U16[1])
			{
				TR.size.U16[0]  = ((TR.size.U16[0] & 0x3FF) + 0x0F) & ~0x0F;
				TR.size.U16[1] &= 0x1FF;
				raster->DrawFill();
			}

			SetReady();
		}
		break;
	}
}