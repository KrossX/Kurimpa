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
#include "GS_GIF_FIFO.h"

void GraphicsSynthesizer::ReadFIFO(u64* mem)
{
	printf("Kurimpa -> GS : ReadFIFO [%X]\n", mem);
}

//Transfer local-host, Reverse FIFO
void GraphicsSynthesizer::ReadFIFO2(u64* mem, u32 qwc)
{					
	// Access FINISH, throw event
	// Wait for FINISH on CSR, then set it to 0
	// Change BUSDIR to 1, transfer, revert BUSDIR to 0
	// During transfer, General register port is not accessible?
	
	//Regs.Priviledged.BUSDIR.DIR = 1;
	// Replace for transfer here.
	//Regs.Priviledged.BUSDIR.DIR = 0;
	
	printf("Kurimpa -> GS : ReadFIFO2 [%X|%u] :: ", mem, qwc);
}

void GraphicsSynthesizer::GIF_Transfer(const u8* mem, u32 size)
{
	printf("Kurimpa -> GS : GIF_Transfer [%X|%X]\n", mem, size);
}

void GraphicsSynthesizer::GIF_Transfer1(u8* mem, u32 addr)
{
	printf("Kurimpa -> GS : GIF_Transfer1 [%X|%X]\n", mem, addr);
}

void GraphicsSynthesizer::GIF_Transfer2(u8* mem, u32 size)
{
	printf("Kurimpa -> GS : GIF_Transfer2 [%X|%u]\n", mem, size);
}

void GraphicsSynthesizer::GIF_Transfer3(u8* mem, u32 size)
{
	printf("Kurimpa -> GS : GIF_Transfer3 [%X|%u]\n", mem, size);
}
