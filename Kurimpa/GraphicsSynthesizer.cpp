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

int GraphicsSynthesizer::Init()
{
	printf("Kurimpa -> GS : Init\n");
	return 0;
}

void GraphicsSynthesizer::Shutdown()
{
	printf("Kurimpa -> GS : Shutdown\n");
}

int GraphicsSynthesizer::Open(void** dsp, char* title, int mt)
{
	printf("Kurimpa -> GS : Open\n");
	return 0;
}

int GraphicsSynthesizer::Open2(void** dsp, u32 flags)
{
	printf("Kurimpa -> GS : Open2\n");
	return 0;
}

void GraphicsSynthesizer::Close()
{
	printf("Kurimpa -> GS : Close\n");
}

void GraphicsSynthesizer::Vsync(int field)
{
	printf("Kurimpa -> GS : Vsync\n");
}

void GraphicsSynthesizer::SetBaseMem(u8* mem)
{	
	printf("Kurimpa -> GS : SetBaseMem\n");
}

void GraphicsSynthesizer::Reset()
{
	printf("Kurimpa -> GS : Reset\n");
}

void GraphicsSynthesizer::WriteCSR(u32 csr)
{
	printf("Kurimpa -> CSR: %08X\n", csr);
}

void GraphicsSynthesizer::GIF_SoftReset(u32 mask)
{
	printf("Kurimpa -> GS : GIF_SoftReset [%X]\n", mask);
}

void GraphicsSynthesizer::IRQ_Callback(void (*irq)())
{
	printf("Kurimpa -> GS : IRQ_Callback\n");
}

void GraphicsSynthesizer::GetLastTag(u32* tag)
{
	printf("Kurimpa -> GS : GetLastTag\n");
}

void GraphicsSynthesizer::GetTitleInfo2(char* dest, size_t length)
{
	printf("Kurimpa -> GS : GetTitleInfo2\n");
}

void GraphicsSynthesizer::SetFrameSkip(int frameskip)
{
	printf("Kurimpa -> GS : SetFrameSkip [%X]\n", frameskip);
}

void GraphicsSynthesizer::SetVsync(int enabled)
{
	printf("Kurimpa -> GS : SetVsync [%X]\n", enabled);
}

void GraphicsSynthesizer::SetExclusive(int enabled)
{
	printf("Kurimpa -> GS : SetExclusive [%X]\n", enabled);
}

void GraphicsSynthesizer::SetFrameLimit(int limit)
{
	printf("Kurimpa -> GS : SetFrameLimit [%X]\n", limit);
}