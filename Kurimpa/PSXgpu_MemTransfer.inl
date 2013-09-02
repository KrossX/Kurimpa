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

void PSXgpu::Transfer_VRAM_VRAM(u32 data)
{
	switch(gpcount)
	{
	case 0:
		DebugFunc();
		//SetReadyCMD(0);
		//SetReadyDMA(0);
		gpsize = 3;
		GPUMODE = GPUMODE_TRANSFER_VRAM_VRAM;
		break;

	case 1: TR.src.U32 = data; break;
	case 2: TR.dst.U32 = data; break;
	case 3:
		{
			TR.SetSize(data);
			u16 sy, sx, dy, dx;

			for(u16 y = 0; y < TR.size.U16[1]; y++)
			{
				sy = (TR.src.U16[1] + y) & 0x1FF;
				dy = (TR.dst.U16[1] + y) & 0x1FF;

				for(u16 x = 0; x < TR.size.U16[0]; x++)
				{
					sx = (TR.src.U16[0] + x) & 0x3FF;
					dx = (TR.dst.U16[0] + x) & 0x3FF;

					if(!doMaskCheck(dx, dy))
					{
						VRAM.HALF2[dy][dx] = VRAM.HALF2[sy][sx];
						if(GPUSTAT.MASKSET)
						VRAM.HALF2[dy][dx] |= 0x8000;
					}
				}
			}

			SetReady();
		}
		break;
	}
}

void PSXgpu::Transfer_CPU_VRAM(u32 data)
{
	static u16 x, y;

	switch(gpcount)
	{
	case 0: 
		DebugFunc();
		//SetReadyCMD(0);
		//SetReadyDMA(1); // Set DMA request? 
		GPUMODE = GPUMODE_TRANSFER_CPU_VRAM;
		break;

	case 1:
		TR.dst.U32 = data;
		break;

	case 2:
		TR.SetSize(data);
		gpsize = TR.GetSize();
		x = 0; y = 0;
		break;
	
	default:
		{
			Word data0; data0.U32 = data;

			for(u8 twice = 0; twice < 2; twice++, x++)
			{
				if(x >= TR.size.U16[0]) { x = 0; y++; };
				if(y >= TR.size.U16[1]) break;

				u16 dx = (TR.dst.U16[0] + x) & 0x3FF;
				u16 dy = (TR.dst.U16[1] + y) & 0x1FF;

				if(!doMaskCheck(dx, dy))
				{
					VRAM.HALF2[dy][dx] = data0.U16[twice];
					if(GPUSTAT.MASKSET)
					VRAM.HALF2[dy][dx] |= 0x8000;
				}
			}

			if(gpcount >= gpsize) SetReady();
		}
		break;
	}
}

void PSXgpu::Transfer_VRAM_CPU(u32 data)
{
	static u16 x, y;

	switch(gpcount)
	{
	case 0:
		DebugFunc();
		//SetReadyCMD(0);
		//SetReadyDMA(1);
		GPUMODE = GPUMODE_TRANSFER_VRAM_CPU;
		break;

	case 1:
		TR.src.U32 = data;
		break;

	case 2:
		TR.SetSize(data);
		gpsize = TR.GetSize();
		GPUSTAT.READYVRAMCPU = 1;
		x = y = 0;
		break;

	default:
		{
			for(u8 twice = 0; twice < 2; twice++, x++)
			{
				if(x >= TR.size.U16[0]) { x = 0; y++; };
				if(y >= TR.size.U16[1]) break;

				u16 dx = (TR.src.U16[0] + x) & 0x3FF;
				u16 dy = (TR.src.U16[1] + y) & 0x1FF;

				TR.dst.U16[twice] = VRAM.HALF2[dy][dx];
			}

			GPUREAD = TR.dst.U32;
			if(gpcount >= gpsize)
			{
				GPUSTAT.READYVRAMCPU = 0;
				SetReady();
			}
		}
		break;
	}
}
