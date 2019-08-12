// 16512 bits per line to make it divisible by 16 and 24
// 1032 in 16bits, 688 in 24bits
//


#define PSX_VRAM_SIZE (1024 * 512 * 2)

enum
{
	GPUMODE_COMMAND = 0,
	GPUMODE_FILL,
	GPUMODE_POLY_F3,
	GPUMODE_POLY_FT3,
	GPUMODE_POLY_F4,
	GPUMODE_POLY_FT4,
	GPUMODE_POLY_G3,
	GPUMODE_POLY_GT3,
	GPUMODE_POLY_G4,
	GPUMODE_POLY_GT4,
	GPUMODE_LINE_F2,
	GPUMODE_LINE_FX,
	GPUMODE_LINE_G2,
	GPUMODE_LINE_GX,
	GPUMODE_TILE,
	GPUMODE_SPRITE,
	GPUMODE_TILE_N,
	GPUMODE_SPRITE_N,
	GPUMODE_COPY_VRAM_VRAM,
	GPUMODE_COPY_CPU_VRAM,
	GPUMODE_COPY_VRAM_CPU,
	GPUMODE_DUMMY
};

s8 dither_matrix[4][4] =
{
	{-4,  0, -3, +1},
	{+2, -2, +3, -1},
	{-3, +1, -4,  0},
	{+3, -1, +2, -2}
};

union psx_convert
{
	u32 raw;

	struct { s32 x : 11, : 5, y : 11; } pos;
	struct { u32 r : 8, g : 8, b : 8; } col;

	struct
	{
		u32 a; //??
	} tpage;

	struct { u32 u : 8, v : 8; } tcoord;
	struct { u32: 16, x : 6, y : 9; } clut;
	struct { u32 mskx : 5, msky : 5, offx : 5, offy : 5; } twin;
	struct { u32 x : 10, y : 10; } clip;
	struct { s32 x : 11, y : 11; } offset;
};

struct psx_color
{
	int r, g, b;
};

struct psx_vertex
{
	int x, y;
	int u, v;
	struct psx_color c;
};

struct psx_gpu
{
	void *vram;
	void *clut;

	u32 status_reg;
	u32 return_data;

	u32 texture_window;
	u32 draw_area_top_left;
	u32 draw_area_bot_right;
	u32 draw_offset;

	u32 mode;
	u32 count;

	// disp_env
	int disp_width;
	int disp_height;
	int screen_width;
	int screen_height;
	int disp_interlaced;
	int disp_24bpp;

	// draw_env
	int clip_top;
	int clip_left;
	int clip_bottom;
	int clip_right;
	int draw_offx;
	int draw_offy;
	int env_dithering;
	int env_replace;
	int env_blending;
	int env_textured;
	int env_gouraud;
	int twin_offx;
	int twin_offy;
	int twin_mskx;
	int twin_msky;
	int rect_flipx;
	int rect_flipy;
	int allow_texdisable;

	u16 draw_mask;
	int maskcheck;

	union psx_convert conv;
	struct psx_vertex vtx[4];
};

static struct psx_gpu gpu;

#define abs(x) (x < 0 ? -x : x)

static
void set_gpu_mode(int mode)
{
	gpu.mode = mode;

	if (mode == GPUMODE_COMMAND)
		gpu.status_reg |=  (1 << 26); // Ready CMD
	else
		gpu.status_reg &= ~(1 << 26);
}

static
void set_vtx_tex(u32 index, u32 data)
{
	gpu.conv.raw = data;
	gpu.vtx[index].u = gpu.conv.tcoord.u;
	gpu.vtx[index].v = gpu.conv.tcoord.v;
}

static
void set_vtx_color(u32 index, u32 data)
{
	gpu.conv.raw = data;
	gpu.vtx[index].c.r = gpu.conv.col.r;
	gpu.vtx[index].c.g = gpu.conv.col.g;
	gpu.vtx[index].c.b = gpu.conv.col.b;
}

static
void set_vtx_pos(u32 index, u32 data)
{
	gpu.conv.raw = data;
	gpu.vtx[index].x = gpu.conv.pos.x;
	gpu.vtx[index].y = gpu.conv.pos.y;
}

static
u16 get_texel(int u, int v)
{
	int tpagex, tpagey, type, idx4, idx8, offx, offy, mskx, msky;
	u16 *clut, *vram16, *tex;
	u16 color_out = 0;
	
	tpagex = (gpu.status_reg & 0xF) * 64;
	tpagey = ((gpu.status_reg >> 4) & 1) * 256;
	type   = (gpu.status_reg >> 7) & 0x3;

	clut   = gpu.clut;
	vram16 = gpu.vram;
	
	//texture window
	offx = (gpu.twin_offx & gpu.twin_mskx) * 8;
	offy = (gpu.twin_offy & gpu.twin_msky) * 8;
	mskx = ~(gpu.twin_mskx * 8);
	msky = ~(gpu.twin_msky * 8);
	
	u = ((u&0xFF) & mskx) | offx;
	v = ((v&0xFF) & msky) | offy;
	
	tex = &vram16[(tpagey + v) * 1024 + tpagex];

	idx4 = (tex[u >> 2] >> ((u & 3) << 2)) & 0x0F;
	idx8 = (tex[u >> 1] >> ((u & 1) << 3)) & 0xFF;

	switch (type)
	{
	case 0: color_out = clut[idx4]; break; // 4-bit
	case 1: color_out = clut[idx8]; break; // 8-bit
	case 2: color_out = tex[u]; break; // 15-bit
	case 3: color_out = tex[u]; break; // reserved (15-bit)
	}
	
	return color_out;
}

static
void set_pixel(int x, int y, struct psx_color *c, u16 texel)
{
	int r, g, b;
	u16 *vram16, *pixel, color, texmask;
	
	x += gpu.draw_offx;
	y += gpu.draw_offy;
	
	x &= 0x3FF;
	y &= 0x1FF;
	
	if (y < gpu.clip_top || y > gpu.clip_bottom || x < gpu.clip_left || x > gpu.clip_right)
		return;

	vram16 = gpu.vram;
	pixel  = &vram16[(y & 0x1FF) * 1024 + (x & 0x3FF)];
	
	if(gpu.maskcheck && (*pixel & 0x8000))
		return;
	
	r = c->r;
	g = c->g;
	b = c->b;
	
	texmask = texel & 0x8000;
	
	if (gpu.env_textured)
	{
		int tr =  texel & 0x1F;
		int tg = (texel >> 5) & 0x1F;
		int tb = (texel >> 10) & 0x1F;
		
		if(gpu.env_replace)
		{
			r = (tr * 0xFF) / 0x1F;
			g = (tg * 0xFF) / 0x1F;
			b = (tb * 0xFF) / 0x1F;
		}
		else
		{
			r = (r * tr) / 16;
			g = (g * tg) / 16;
			b = (b * tb) / 16;
			
			r = r > 0xFF ? 0xFF : r;
			g = g > 0xFF ? 0xFF : g;
			b = b > 0xFF ? 0xFF : b;
		}
	}
	
	if (gpu.env_dithering)
	{
		s8 offset = dither_matrix[y&3][x&3];

		r += offset;
		g += offset;
		b += offset;

		r = r > 0xFF ? 0xFF : r < 0 ? 0 : r;
		g = g > 0xFF ? 0xFF : g < 0 ? 0 : g;
		b = b > 0xFF ? 0xFF : b < 0 ? 0 : b;
	}
	
	r >>= 3;
	g >>= 3;
	b >>= 3;
	
	if (gpu.env_blending && (!gpu.env_textured || texmask))
	{
		int dr = (*pixel) & 0x1F;
		int dg = (*pixel >> 5) & 0x1F;
		int db = (*pixel >> 10) & 0x1F;
	
		switch((gpu.status_reg >> 5)&3)
		{
		case 0: // D/2 + S/2
			r = (dr + r) >> 1;
			g = (dg + g) >> 1;
			b = (db + b) >> 1;
			break;
	
		case 1: // D + S
			r = dr + r;
			g = dg + g;
			b = db + b;

			r = r > 0x1F ? 0x1F : r;
			g = g > 0x1F ? 0x1F : g;
			b = b > 0x1F ? 0x1F : b;
			break;
	
		case 2: // D - S
			r = dr - r;
			g = dg - g;
			b = db - b;

			r = r < 0 ? 0 : r;
			g = g < 0 ? 0 : g;
			b = b < 0 ? 0 : b;
			break;

		case 3: // D + S/4
			r = dr + (r >> 2);
			g = dg + (g >> 2);
			b = db + (b >> 2);

			r = r > 0x1F ? 0x1F : r;
			g = g > 0x1F ? 0x1F : g;
			b = b > 0x1F ? 0x1F : b;
			break;
		}
	}
	
	color = r | (g << 5) | (b << 10);

	*pixel = color | gpu.draw_mask | texmask;
}

static
u16 get_texel_st(float s, float t, struct psx_vertex *vtx1, struct psx_vertex *vtx2, struct psx_vertex *vtx3)
{
	float st = 1.0f - s - t;
	int u = (int)(vtx1->u * st + vtx2->u * s + vtx3->u * t + 0.5f);
	int v = (int)(vtx1->v * st + vtx2->v * s + vtx3->v * t + 0.5f);
	
	return get_texel(u, v);
}

static
struct psx_color* get_gouraud(float s, float t, struct psx_color *c0, struct psx_color *c1, struct psx_color *c2)
{
	static struct psx_color c;
	float st = 1.0f - (s+t);

	c.r = (u8)(c0->r * st + c1->r * s + c2->r * t + 0.5f);
	c.g = (u8)(c0->g * st + c1->g * s + c2->g * t + 0.5f);
	c.b = (u8)(c0->b * st + c1->b * s + c2->b * t + 0.5f);

	return &c;
}

static
void draw_triangle(struct psx_vertex *vtx1, struct psx_vertex *vtx2, struct psx_vertex *vtx3)
{
	int minx, miny, maxx, maxy;
	int width, height;
	int STDX1, STDY1, STDX2, STDY2, CROSS;
	int X1, Y1, X2, Y2, X3, Y3;
	int DX12, DX23, DX31;
	int DY12, DY23, DY31;
	int FDX12, FDX23, FDX31;
	int FDY12, FDY23, FDY31;
	int C1, C2, C3;
	int CY1, CY2, CY3;
	int CX1, CX2, CX3;
	int x, y;
	int dither;

	int CSDYX, CTDYX;
	float CSY, CSX, CS;
	float CTY, CTX, CT;
	
	struct psx_vertex *vA = vtx1;
	struct psx_vertex *vB = vtx2;
	struct psx_vertex *vC = vtx3;
	
	minx = min(min(vtx1->x, vtx2->x), vtx3->x);
	maxx = max(max(vtx1->x, vtx2->x), vtx3->x);
	miny = min(min(vtx1->y, vtx2->y), vtx3->y);
	maxy = max(max(vtx1->y, vtx2->y), vtx3->y);

	width = maxx - minx;
	height = maxy - miny;

	if (width > 1023) return;
	if (height > 511) return;

	STDX1 = vtx2->x - vtx1->x;
	STDX2 = vtx3->x - vtx1->x;
	STDY1 = vtx2->y - vtx1->y;
	STDY2 = vtx3->y - vtx1->y;

	CROSS = (STDX2 * STDY1) - (STDY2 * STDX1);
	if (!CROSS) return;
	
	CSDYX = vtx1->y * STDX2 - vtx1->x * STDY2; 
	CTDYX = vtx1->y * STDX1 - vtx1->x * STDY1; 
	
	CSY = (float)STDY2 / -CROSS;
	CSX = (float)STDX2 / -CROSS;
	CS  = (float)CSDYX / -CROSS;
	
	CTY = (float)STDY1 /  CROSS;
	CTX = (float)STDX1 /  CROSS;
	CT  = (float)CTDYX /  CROSS;
	
	if (CROSS < 0) { vA = vtx3; vC = vtx1; }

	// 28.4 fixed-point coordinates
	Y1 = vA->y << 4;
	Y2 = vB->y << 4;
	Y3 = vC->y << 4;

	X1 = vA->x << 4;
	X2 = vB->x << 4;
	X3 = vC->x << 4;

	// Deltas
	DX12 = X1 - X2;
	DX23 = X2 - X3;
	DX31 = X3 - X1;

	DY12 = Y1 - Y2;
	DY23 = Y2 - Y3;
	DY31 = Y3 - Y1;

	// Fixed-point deltas
	FDX12 = DX12 << 4;
	FDX23 = DX23 << 4;
	FDX31 = DX31 << 4;

	FDY12 = DY12 << 4;
	FDY23 = DY23 << 4;
	FDY31 = DY31 << 4;

	// Half-edge constants
	C1 = DY12 * X1 - DX12 * Y1;
	C2 = DY23 * X2 - DX23 * Y2;
	C3 = DY31 * X3 - DX31 * Y3;

	// Correct for fill convention
	if (DY12 < 0 || (DY12 == 0 && DX12 > 0)) C1++;
	if (DY23 < 0 || (DY23 == 0 && DX23 > 0)) C2++;
	if (DY31 < 0 || (DY31 == 0 && DX31 > 0)) C3++;

	CY1 = C1 + DX12 * (miny << 4) - DY12 * (minx << 4);
	CY2 = C2 + DX23 * (miny << 4) - DY23 * (minx << 4);
	CY3 = C3 + DX31 * (miny << 4) - DY31 * (minx << 4);
	
	dither = gpu.env_dithering;
	
	if(!gpu.env_gouraud || (gpu.env_textured && gpu.env_replace))
		gpu.env_dithering = 0;
	else
		gpu.env_dithering = dither;
	
	if(gpu.env_gouraud)
	{
		if(gpu.env_textured)
		{
			for (y = miny; y < maxy; y++)
			{
				CX1 = CY1;
				CX2 = CY2;
				CX3 = CY3;

				for (x = minx; x < maxx; x++)
				{
					if (CX1 > 0 && CX2 > 0 && CX3 > 0)
					{
						float s = x * CSY - y * CSX + CS;
						float t = x * CTY - y * CTX + CT;

						u16 texel = get_texel_st(s, t, vtx1, vtx2, vtx3);
						if(texel) set_pixel(x, y, get_gouraud(s, t, &vtx1->c, &vtx2->c, &vtx3->c), texel);
					}

					CX1 -= FDY12;
					CX2 -= FDY23;
					CX3 -= FDY31;
				}

				CY1 += FDX12;
				CY2 += FDX23;
				CY3 += FDX31;
			}
		}
		else
		{
			for (y = miny; y < maxy; y++)
			{
				CX1 = CY1;
				CX2 = CY2;
				CX3 = CY3;

				for (x = minx; x < maxx; x++)
				{
					if (CX1 > 0 && CX2 > 0 && CX3 > 0)
					{
						float s = x * CSY - y * CSX + CS;
						float t = x * CTY - y * CTX + CT;
						
						set_pixel(x, y, get_gouraud(s, t, &vtx1->c, &vtx2->c, &vtx3->c), 0);
					}

					CX1 -= FDY12;
					CX2 -= FDY23;
					CX3 -= FDY31;
				}

				CY1 += FDX12;
				CY2 += FDX23;
				CY3 += FDX31;
			}
		}

	}
	else
	{
		if(gpu.env_textured)
		{
			for (y = miny; y < maxy; y++)
			{
				CX1 = CY1;
				CX2 = CY2;
				CX3 = CY3;

				for (x = minx; x < maxx; x++)
				{
					if (CX1 > 0 && CX2 > 0 && CX3 > 0)
					{
						float s = x * CSY - y * CSX + CS;
						float t = x * CTY - y * CTX + CT;

						u16 texel = get_texel_st(s, t, vtx1, vtx2, vtx3);
						if(texel) set_pixel(x, y, &gpu.vtx[0].c, texel);
					}

					CX1 -= FDY12;
					CX2 -= FDY23;
					CX3 -= FDY31;
				}

				CY1 += FDX12;
				CY2 += FDX23;
				CY3 += FDX31;
			}
		}
		else
		{
			for (y = miny; y < maxy; y++)
			{
				CX1 = CY1;
				CX2 = CY2;
				CX3 = CY3;

				for (x = minx; x < maxx; x++)
				{
					if (CX1 > 0 && CX2 > 0 && CX3 > 0)
					{
						set_pixel(x, y, &gpu.vtx[0].c, 0);
					}

					CX1 -= FDY12;
					CX2 -= FDY23;
					CX3 -= FDY31;
				}

				CY1 += FDX12;
				CY2 += FDX23;
				CY3 += FDX31;
			}
		}
	}
	
	gpu.env_dithering = dither;
}

static
int floor_int16(double x)
{
	return (int)(x + 32767.5) - 32767;
}

static
void draw_line(struct psx_vertex *vtx1, struct psx_vertex *vtx2)
{
	int length, i, dx, dy, dr, dg, db;
	double x, y;
	double x_inc, y_inc;
	double r_inc, g_inc, b_inc;
	
	dx = vtx2->x - vtx1->x;
	dy = vtx2->y - vtx1->y;
	
	dr = vtx2->c.r - vtx1->c.r;
	dg = vtx2->c.g - vtx1->c.g;
	db = vtx2->c.b - vtx1->c.b;
	
	if(abs(dx) > 1023 || abs(dy) > 511) return;
	
	length = abs(dy) > abs(dx) ? abs(dy) : abs(dx);

	x_inc = (double)(dx)/(double)length;
	y_inc = (double)(dy)/(double)length;

	r_inc = (double)(dr)/(double)length;
	g_inc = (double)(dg)/(double)length;
	b_inc = (double)(db)/(double)length;

	x = vtx1->x + ((dx > 0) ? 0 : -0.05); 
	y = vtx1->y + ((dy > 0) ? 0 : -0.05);

	if ((dr|dg|db) == 0)
	{
		for (i = 0; i <= length; i++)
		{
			set_pixel(floor_int16(x), floor_int16(y), &gpu.vtx[0].c, 0);
			x = x + x_inc;
			y = y + y_inc;
		}
	}
	else
	{
		struct psx_color c;
		
		double r = vtx1->c.r + 0.5;
		double g = vtx1->c.g + 0.5;
		double b = vtx1->c.b + 0.5;
		
		for (i = 0; i <= length; i++)
		{
			c.r = (int)r;
			c.g = (int)g;
			c.b = (int)b;
			
			set_pixel(floor_int16(x), floor_int16(y), &c, 0);
			x += x_inc;
			y += y_inc;
			
			r += r_inc;
			g += g_inc;
			b += b_inc;
		}
	}
	
	LOG_PRINTF(message[message_idx++], "LINE (%d, %d) (%d, %d)\n", vtx1->x, vtx1->y, vtx2->x, vtx2->y);
}

static
void draw_rect(int width, int height)
{
	int sx = gpu.vtx[0].x;
	int sy = gpu.vtx[0].y;

	int endx = sx + width;
	int endy = sy + height;
	
	int x, y, u, v;
	
	int dither = gpu.env_dithering;
	gpu.env_dithering = 0;
	
	if (gpu.env_textured)
	{
		int su = gpu.vtx[0].u;
		int sv = gpu.vtx[0].v;
		int incu = 1;
		int incv = 1;
		int tex  = (gpu.status_reg >> 7) & 0x3;
		int oddmask = tex == 0 ? 0xF : tex == 1 ? 0x7 : 0x3;
		int oddfix = gpu.rect_flipx ? 0 : su&1;
		
		if(gpu.rect_flipx) { incu = -1; su |= 1; }
		if(gpu.rect_flipy) { incv = -1; }
		
		for (y = sy, v = sv; y < endy; y++, v+=incv)
			for (x = sx, u = su; x < endx; x++, u+=incu)
			{
				int uF = ((u&oddmask) == 0) ? 1 : 0;
				u16 texel = get_texel((oddfix && uF) ? u - incu : u, v);
				if(texel) set_pixel(x, y, &gpu.vtx[0].c, texel);
			}
			
		LOG_PRINTF(message[message_idx++], "SPRITE (%d, %d) (%d, %d) (%d, %d) %d\n", width, height, su, sv,
			incu, incv, oddfix);
	}
	else
	{
		for (y = sy; y < endy; y++)
			for (x = sx; x < endx; x++)
			{
				set_pixel(x, y, &gpu.vtx[0].c, 0);
			}
	}
	
	gpu.env_dithering = dither;
}

static
void gpu_fill(u32 data)
{
	int x, y;
	u16 col16;
	
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_FILL);
		set_vtx_color(0, data);
		break;

	case 1:
		gpu.vtx[0].x = data & 0x3F0;
		gpu.vtx[0].y = (data >> 16) & 0x1FF;
		break;

	case 2:
		gpu.vtx[1].x = ((data & 0x3FF) + 0x0F) & ~0x0F;
		gpu.vtx[1].y = (data >> 16) & 0x1FF;
		
		col16  = (gpu.vtx[0].c.r >> 3);
		col16 |= (gpu.vtx[0].c.g >> 3) << 5;
		col16 |= (gpu.vtx[0].c.b >> 3) << 10;

		if (gpu.vtx[1].x && gpu.vtx[1].y)
		{
			u16 *vram16 = gpu.vram;

			int sx = gpu.vtx[0].x;
			int sy = gpu.vtx[0].y;

			int endx = sx + gpu.vtx[1].x;
			int endy = sy + gpu.vtx[1].y;

			for (y = sy; y < endy; y++)
			for (x = sx; x < endx; x++)
			{
				vram16[y * 1024 + x] = col16;
			}
		}

		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

//0   // 0: modulate 1: replace
//1   // 1: do blend (mask?)
//2   // 1: textured
//3   // 0: triangle 1: quad
//4   // 0: flat 1: shaded
//5.6 // 1: polygon, 2: line, 3: rects

static
void gpu_poly_f3(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_F3);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_pos(1, data);
		break;

	case 3:
		set_vtx_pos(2, data);

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_ft3(u32 data)
{
	static u32 tpage;

	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_FT3);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 1;
		gpu.env_gouraud  = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_tex(0, data);
		gpu.clut = (u8*)gpu.vram + (gpu.conv.clut.y << 11) + (gpu.conv.clut.x << 5);
		break;

	case 3:
		set_vtx_pos(1, data);
		break;

	case 4:
		set_vtx_tex(1, data);
		tpage = (data >> 16) & 0x1FF;
		break;

	case 5:
		set_vtx_pos(2, data);
		break;

	case 6:
		set_vtx_tex(2, data);

		gpu.status_reg = (gpu.status_reg & ~0x1FF) | tpage;

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_f4(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_F4);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_pos(1, data);
		break;

	case 3:
		set_vtx_pos(2, data);
		break;

	case 4:
		set_vtx_pos(3, data);

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);
		draw_triangle(&gpu.vtx[1], &gpu.vtx[2], &gpu.vtx[3]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_ft4(u32 data)
{
	static u32 tpage;

	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_FT4);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 1;
		gpu.env_gouraud  = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_tex(0, data);
		gpu.clut = (u8*)gpu.vram + (gpu.conv.clut.y << 11) + (gpu.conv.clut.x << 5);
		break;

	case 3:
		set_vtx_pos(1, data);
		break;

	case 4:
		set_vtx_tex(1, data);
		tpage = (data >> 16) & 0x1FF;
		break;

	case 5:
		set_vtx_pos(2, data);
		break;

	case 6:
		set_vtx_tex(2, data);
		gpu.status_reg = (gpu.status_reg & ~0x1FF) | tpage;
		break;

	case 7:
		set_vtx_pos(3, data);
		break;

	case 8:
		set_vtx_tex(3, data);

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);
		draw_triangle(&gpu.vtx[1], &gpu.vtx[2], &gpu.vtx[3]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_g3(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_G3);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_color(1, data);
		break;

	case 3:
		set_vtx_pos(1, data);
		break;

	case 4:
		set_vtx_color(2, data);
		break;

	case 5:
		set_vtx_pos(2, data);

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_gt3(u32 data)
{
	static u32 tpage;

	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_GT3);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 1;
		gpu.env_gouraud  = 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_tex(0, data);
		gpu.clut = (u8*)gpu.vram + (gpu.conv.clut.y << 11) + (gpu.conv.clut.x << 5);
		break;

	case 3:
		set_vtx_color(1, data);
		break;

	case 4:
		set_vtx_pos(1, data);
		break;

	case 5:
		set_vtx_tex(1, data);
		tpage = (data >> 16) & 0x1FF;
		break;

	case 6:
		set_vtx_color(2, data);
		break;

	case 7:
		set_vtx_pos(2, data);
		break;

	case 8:
		set_vtx_tex(2, data);

		gpu.status_reg = (gpu.status_reg & ~0x1FF) | tpage;

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_g4(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_G4);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_color(1, data);
		break;

	case 3:
		set_vtx_pos(1, data);
		break;

	case 4:
		set_vtx_color(2, data);
		break;

	case 5:
		set_vtx_pos(2, data);
		break;

	case 6:
		set_vtx_color(3, data);
		break;

	case 7:
		set_vtx_pos(3, data);

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);
		draw_triangle(&gpu.vtx[1], &gpu.vtx[2], &gpu.vtx[3]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_poly_gt4(u32 data)
{
	static u32 tpage;

	switch (gpu.count)
	{
	case 0:
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_gpu_mode(GPUMODE_POLY_GT4);
		set_vtx_color(0, data);
		
		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 1;
		gpu.env_gouraud  = 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_tex(0, data);
		gpu.clut = (u8*)gpu.vram + (gpu.conv.clut.y << 11) + (gpu.conv.clut.x << 5);
		break;

	case 3:
		set_vtx_color(1, data);
		break;

	case 4:
		set_vtx_pos(1, data);
		break;

	case 5:
		set_vtx_tex(1, data);
		tpage = (data >> 16) & 0x1FF;
		break;

	case 6:
		set_vtx_color(2, data);
		break;

	case 7:
		set_vtx_pos(2, data);
		break;

	case 8:
		set_vtx_tex(2, data);
		gpu.status_reg = (gpu.status_reg & ~0x1FF) | tpage;
		break;

	case 9:
		set_vtx_color(3, data);
		break;

	case 10:
		set_vtx_pos(3, data);
		break;

	case 11:
		set_vtx_tex(3, data);

		draw_triangle(&gpu.vtx[0], &gpu.vtx[1], &gpu.vtx[2]);
		draw_triangle(&gpu.vtx[1], &gpu.vtx[2], &gpu.vtx[3]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

//0   // ??
//1   // 1: do blend (mask?)
//2   // ??
//3   // 0: single line 1: variable length
//4   // 0: flat 1: shaded
//5.6 // 1: polygon, 2: line, 3: rects

static
void gpu_line_f2(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_LINE_F2);
		gpu.status_reg &= ~(1 << 28); // ready dma

		set_vtx_color(0, data);
		set_vtx_color(1, data);
		
		gpu.env_replace  = 0;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_pos(1, data);

		draw_line(&gpu.vtx[0], &gpu.vtx[1]);

		
		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_line_fx(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_LINE_FX);
		gpu.status_reg &= ~(1 << 28); // ready dma

		set_vtx_color(0, data);
		set_vtx_color(1, data);

		gpu.env_replace  = 0;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_pos(1, data);

		draw_line(&gpu.vtx[0], &gpu.vtx[1]);
		break;

	default:
		if ((data & 0xF000F000) == 0x50005000)
		{
			gpu.status_reg |= (1 << 28);
			set_gpu_mode(GPUMODE_COMMAND);
		}
		else
		{
			int index = (gpu.count - 1) & 1;

			set_vtx_pos(index, data);

			draw_line(&gpu.vtx[index ^ 1], &gpu.vtx[index]);
		}
		break;
	}
}

static
void gpu_line_g2(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_LINE_G2);
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_vtx_color(0, data);

		gpu.env_replace  = 0;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_color(1, data);
		break;

	case 3:
		set_vtx_pos(1, data);

		draw_line(&gpu.vtx[0], &gpu.vtx[1]);

		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_line_gx(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_LINE_GX);
		gpu.status_reg &= ~(1 << 28); // ready dma
		set_vtx_color(0, data);

		gpu.env_replace  = 0;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		gpu.env_gouraud  = 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_color(1, data);
		break;

	case 3:
		set_vtx_pos(1, data);

		draw_line(&gpu.vtx[0], &gpu.vtx[1]);
		break;

	default:
		if ((data & 0xF000F000) == 0x50005000)
		{
			if (gpu.count&1)
			{
				gpu.mode = GPUMODE_DUMMY;
			}
			else
			{
				gpu.status_reg |= (1 << 28);
				set_gpu_mode(GPUMODE_COMMAND);
			}
		}
		else
		{
			int index = (gpu.count >> 1) & 1;

			if (gpu.count & 1)
			{
				set_vtx_pos(index, data);

				draw_line(&gpu.vtx[index ^ 1], &gpu.vtx[index]);
			}
			else
			{
				set_vtx_color(index, data);
			}
		}
		break;
	}
}


//0   // 0: modulate 1: replace
//1   // 1: do blend (mask?)
//2   // 1: textured
//3.4 // 0: variable size 1: 1x1 2: 8x8 3: 16x16
//5.6 // 1: polygon, 2: line, 3: rects

static
void gpu_tile(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_TILE);
		set_vtx_color(0, data);

		gpu.env_replace  = 0;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		draw_rect(data & 0xFFFF, data >> 16);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_tile_n(u32 data)
{
	static u32 rectsize;

	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_TILE_N);
		set_vtx_color(0, data);
		
		rectsize = (data >> 27) & 3;
		rectsize = rectsize == 1 ? 1 : 1 << (rectsize + 1);

		gpu.env_replace  = 0;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = 0;
		break;

	case 1:
		set_vtx_pos(0, data);
		draw_rect(rectsize, rectsize);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_sprite(u32 data)
{
	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_SPRITE);
		set_vtx_color(0, data);

		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = (data >> 26) & 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_tex(0, data);
		gpu.clut = (u8*)gpu.vram + (gpu.conv.clut.y << 11) + (gpu.conv.clut.x << 5);
		break;

	case 3:
		draw_rect(data & 0xFFFF, data >> 16);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void gpu_sprite_n(u32 data)
{
	static u32 rectsize;

	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_SPRITE_N);
		set_vtx_color(0, data);
		
		rectsize = (data >> 27) & 3;
		rectsize = rectsize == 1 ? 1 : 1 << (rectsize + 1);

		gpu.env_replace  = (data >> 24) & 1;
		gpu.env_blending = (data >> 25) & 1;
		gpu.env_textured = (data >> 26) & 1;
		break;

	case 1:
		set_vtx_pos(0, data);
		break;

	case 2:
		set_vtx_tex(0, data);
		gpu.clut = (u8*)gpu.vram + (gpu.conv.clut.y << 11) + (gpu.conv.clut.x << 5);
		draw_rect(rectsize, rectsize);
		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void copy_vram_vram(u32 data)
{
	static u32 srcx, srcy;
	static u32 dstx, dsty;
	static u32 width, height;

	static u32 px, py;
	static u16 *vram16;
	static u16 *srcline;
	static u16 *dstline;
	
	u16 *pixel;

	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_COPY_VRAM_VRAM);
		break;

	case 1:
		srcx = (data & 0xFFFF) & 0x3FF;
		srcy = (data >> 16) & 0x1FF;
		break;

	case 2:
		dstx = (data & 0xFFFF) & 0x3FF;
		dsty = (data >> 16) & 0x1FF;
		break;

	case 3:
		width = data & 0xFFFF;
		height = data >> 16;
		width = ((width - 1) & 0x3FF) + 1;
		height = ((height - 1) & 0x1FF) + 1;

		py = 0;
		vram16 = gpu.vram;

		while (py < height)
		{
			srcline = &vram16[((srcy + py) & 0x1FF) * 1024];
			dstline = &vram16[((dsty + py) & 0x1FF) * 1024];

			px = 0;

			while (px < width)
			{
				pixel = &dstline[(dstx + px) & 0x3FF];
					
				if (gpu.maskcheck == 0 || (*pixel & 0x8000) == 0)
					*pixel = srcline[(srcx + px) & 0x3FF] | gpu.draw_mask;

				px++;
			}

			py++;
		}

		set_gpu_mode(GPUMODE_COMMAND);
		break;
	}
}

static
void copy_cpu_vram(u32 data)
{
	static u32 dstx, dsty;
	static u32 width, height;

	static u32 endcount;
	static u32 px, py;
	static u16 *vram16;
	static u16 *line;

	static u32 tmax, tcount;

	u16 *pixel;

	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_COPY_CPU_VRAM);
		break;

	case 1:
		dstx = (data & 0xFFFF) & 0x3FF;
		dsty = (data >> 16) & 0x1FF;
		break;

	case 2:
		width = data & 0xFFFF;
		height = data >> 16;
		width = ((width - 1) & 0x3FF) + 1;
		height = ((height - 1) & 0x1FF) + 1;
		endcount = gpu.count + ((width * height + 1) >> 1);

		tcount = 0;
		tmax = width * height;

		px = 0;
		py = 1;

		vram16 = (u16*)gpu.vram;
		line = &vram16[dsty * 1024];
		break;

	default:
		pixel = &line[(dstx + px++) & 0x3FF];

		if (gpu.maskcheck == 0 || (*pixel & 0x8000) == 0)
			*pixel = (u16)((data & 0xFFFF) | gpu.draw_mask);

		if (px >= width && py < height)
		{
			px = 0;
			line = &vram16[((dsty + py++) & 0x1FF) * 1024];
		}

		if (++tcount < tmax)
		{
			pixel = &line[(dstx + px++) & 0x3FF];
			if (gpu.maskcheck == 0 || (*pixel & 0x8000) == 0)
				*pixel = (u16)((data >> 16) | gpu.draw_mask);

			if (px >= width && py < height)
			{
				px = 0;
				line = &vram16[((dsty + py++) & 0x1FF) * 1024];
			}
		}

		if (++tcount >= tmax)
		{
			set_gpu_mode(GPUMODE_COMMAND);
			return;
		}
		break;
	}
}

static
void copy_vram_cpu(u32 data)
{
	static u32 srcx, srcy;
	static u32 width, height;
	static u32 endcount;

	static u32 px, py;
	static u16 *vram16;
	static u16 *line;

	static u32 tmax, tcount;

	switch (gpu.count)
	{
	case 0:
		set_gpu_mode(GPUMODE_COPY_VRAM_CPU);
		compare_mem(gpu.vram, plugin_mem);
		break;

	case 1:
		srcx = (data & 0xFFFF) & 0x3FF;
		srcy = (data >> 16) & 0x1FF;
		break;

	case 2:
		width = data & 0xFFFF;
		height = data >> 16;
		width = ((width - 1) & 0x3FF) + 1;
		height = ((height - 1) & 0x1FF) + 1;
		endcount = gpu.count + ((width * height + 1) >> 1);

		tcount = 0;
		tmax =  width * height;

		px = 0;
		py = 1;

		vram16 = (u16*)gpu.vram;
		line = &vram16[(srcy & 0x1FF) * 1024];

		gpu.status_reg = gpu.status_reg | (1 << 27);     // Ready VRAM-CPU
		if (((gpu.status_reg >> 29) & 3) == 3)           // DMA mode 3 GPUREAD to CPU
			gpu.status_reg = gpu.status_reg | (1 << 25); // 25 mirrors 27
		break;

	default:
		gpu.return_data = line[(srcx + px++) & 0x3FF];

		if (px >= width && py < height)
		{
			px = 0;
			line = &vram16[((srcy + py++) & 0x1FF) * 1024];
		}

		tcount++;

		gpu.return_data |= line[(srcx + px++) & 0x3FF] << 16;

		if (px >= width && py < height)
		{
			px = 0;
			line = &vram16[((srcy + py++) & 0x1FF) * 1024];
		}

		if (gpu.return_data == 0x709E709E)
			tmax = tmax;

		if (++tcount >= tmax)
		{
			gpu.status_reg = gpu.status_reg & ~(1 << 27);     // Ready VRAM-CPU
			if (((gpu.status_reg >> 29) & 3) == 3)            // DMA mode 3 GPUREAD to CPU
				gpu.status_reg = gpu.status_reg & ~(1 << 25); // 25 mirrors 27

			set_gpu_mode(GPUMODE_COMMAND);
		}
		break;
	}
}

static
void gpu_command(u32 data)
{
	u8 cmd = (u8)(data >> 24);
	last_cmd = cmd;
	
	gpu.count = 0;
	
	LOG_PRINTF(message[message_idx++], "GPUCMD %08X\n", data);

	switch (cmd)
	{
	case 0x00: return; //nop
	case 0x01: return; //clear cache?
	case 0x02: gpu_fill(data); return;

	case 0x1F: gpu.status_reg |= (1 << 24); return;

	case 0x20: case 0x21: case 0x22:case 0x23: gpu_poly_f3(data);  return;
	case 0x24: case 0x25: case 0x26:case 0x27: gpu_poly_ft3(data); return;
	case 0x28: case 0x29: case 0x2A:case 0x2B: gpu_poly_f4(data);  return;
	case 0x2C: case 0x2D: case 0x2E:case 0x2F: gpu_poly_ft4(data); return;
	case 0x30: case 0x31: case 0x32:case 0x33: gpu_poly_g3(data);  return;
	case 0x34: case 0x35: case 0x36:case 0x37: gpu_poly_gt3(data); return;
	case 0x38: case 0x39: case 0x3A:case 0x3B: gpu_poly_g4(data);  return;
	case 0x3C: case 0x3D: case 0x3E:case 0x3F: gpu_poly_gt4(data); return;

	case 0x40: case 0x41: case 0x42:case 0x43: gpu_line_f2(data); return;
	case 0x44: case 0x45: case 0x46:case 0x47: gpu_line_f2(data); return; // ?? textured
	case 0x48: case 0x49: case 0x4A:case 0x4B: gpu_line_fx(data); return;
	case 0x4C: case 0x4D: case 0x4E:case 0x4F: gpu_line_fx(data); return; // ?? textured
	case 0x50: case 0x51: case 0x52:case 0x53: gpu_line_g2(data); return;
	case 0x54: case 0x55: case 0x56:case 0x57: gpu_line_g2(data); return; // ?? textured
	case 0x58: case 0x59: case 0x5A:case 0x5B: gpu_line_gx(data); return;
	case 0x5C: case 0x5D: case 0x5E:case 0x5F: gpu_line_gx(data); return; // ?? textured

	case 0x60: case 0x61: case 0x62:case 0x63: gpu_tile(data);     return;
	case 0x64: case 0x65: case 0x66:case 0x67: gpu_sprite(data);   return;
	case 0x68: case 0x69: case 0x6A:case 0x6B: gpu_tile_n(data);   return;
	case 0x6C: case 0x6D: case 0x6E:case 0x6F: gpu_sprite_n(data); return; // ?? 1x1 sprite
	case 0x70: case 0x71: case 0x72:case 0x73: gpu_tile_n(data);   return;
	case 0x74: case 0x75: case 0x76:case 0x77: gpu_sprite_n(data); return;
	case 0x78: case 0x79: case 0x7A:case 0x7B: gpu_tile_n(data);   return;
	case 0x7C: case 0x7D: case 0x7E:case 0x7F: gpu_sprite_n(data); return;

	case 0x80: case 0x81: case 0x82:case 0x83: copy_vram_vram(data); return;
	case 0x84: case 0x85: case 0x86:case 0x87: copy_vram_vram(data); return;
	case 0x88: case 0x89: case 0x8A:case 0x8B: copy_vram_vram(data); return;
	case 0x8C: case 0x8D: case 0x8E:case 0x8F: copy_vram_vram(data); return;
	case 0x90: case 0x91: case 0x92:case 0x93: copy_vram_vram(data); return;
	case 0x94: case 0x95: case 0x96:case 0x97: copy_vram_vram(data); return;
	case 0x98: case 0x99: case 0x9A:case 0x9B: copy_vram_vram(data); return;
	case 0x9C: case 0x9D: case 0x9E:case 0x9F: copy_vram_vram(data); return;

	case 0xA0: case 0xA1: case 0xA2:case 0xA3: copy_cpu_vram(data); return;
	case 0xA4: case 0xA5: case 0xA6:case 0xA7: copy_cpu_vram(data); return;
	case 0xA8: case 0xA9: case 0xAA:case 0xAB: copy_cpu_vram(data); return;
	case 0xAC: case 0xAD: case 0xAE:case 0xAF: copy_cpu_vram(data); return;
	case 0xB0: case 0xB1: case 0xB2:case 0xB3: copy_cpu_vram(data); return;
	case 0xB4: case 0xB5: case 0xB6:case 0xB7: copy_cpu_vram(data); return;
	case 0xB8: case 0xB9: case 0xBA:case 0xBB: copy_cpu_vram(data); return;
	case 0xBC: case 0xBD: case 0xBE:case 0xBF: copy_cpu_vram(data); return;

	case 0xC0: case 0xC1: case 0xC2:case 0xC3: copy_vram_cpu(data); return;
	case 0xC4: case 0xC5: case 0xC6:case 0xC7: copy_vram_cpu(data); return;
	case 0xC8: case 0xC9: case 0xCA:case 0xCB: copy_vram_cpu(data); return;
	case 0xCC: case 0xCD: case 0xCE:case 0xCF: copy_vram_cpu(data); return;
	case 0xD0: case 0xD1: case 0xD2:case 0xD3: copy_vram_cpu(data); return;
	case 0xD4: case 0xD5: case 0xD6:case 0xD7: copy_vram_cpu(data); return;
	case 0xD8: case 0xD9: case 0xDA:case 0xDB: copy_vram_cpu(data); return;
	case 0xDC: case 0xDD: case 0xDE:case 0xDF: copy_vram_cpu(data); return;

	case 0xE1:
		gpu.status_reg &= ~0x87FF;
		gpu.status_reg |= data & 0x7FF;

		if(gpu.allow_texdisable)
			gpu.status_reg |= (data & 0x800) << 4;
	
		gpu.env_dithering = (gpu.status_reg >> 9) & 1;
		
		gpu.rect_flipx = (data >> 12) & 1;
		gpu.rect_flipy = (data >> 13) & 1;
		return;

	case 0xE2:
		gpu.texture_window = data & 0xFFFFF;
		gpu.conv.raw = data;
		gpu.twin_mskx = gpu.conv.twin.mskx;
		gpu.twin_msky = gpu.conv.twin.msky;
		gpu.twin_offx = gpu.conv.twin.offx;
		gpu.twin_offy = gpu.conv.twin.offy;
		return;

	case 0xE3:
		gpu.draw_area_top_left = data & 0xFFFFF;
		gpu.conv.raw = data;
		gpu.clip_top = gpu.conv.clip.y;
		gpu.clip_left = gpu.conv.clip.x;
		return;

	case 0xE4:
		gpu.draw_area_bot_right = data & 0xFFFFF;
		gpu.conv.raw = data;
		gpu.clip_bottom = gpu.conv.clip.y;
		gpu.clip_right = gpu.conv.clip.x;
		return;

	case 0xE5:
		gpu.draw_offset = data & 0x3FFFFF;
		gpu.conv.raw = data;
		gpu.draw_offx = gpu.conv.offset.x;
		gpu.draw_offy = gpu.conv.offset.y;
		return;

	case 0xE6:
		gpu.status_reg &= ~0x1800;
		gpu.status_reg |= (data & 3) << 11;
		gpu.draw_mask = (u16)((data & 1) << 15);
		gpu.maskcheck = (data >> 1) & 1;
		return;
	}
}

static
void gpu_data(u32 data)
{
	//LOG_PRINTF(message[message_idx++], "GPUDATA %08X (%02d|%02d)\n", data, gpu.mode, gpu.count);
	
	switch (gpu.mode)
	{
	case GPUMODE_COMMAND: gpu_command(data); break;
	case GPUMODE_FILL:    gpu_fill(data); break;

	case GPUMODE_POLY_F3:  gpu_poly_f3(data);  break;
	case GPUMODE_POLY_FT3: gpu_poly_ft3(data); break;
	case GPUMODE_POLY_F4:  gpu_poly_f4(data);  break;
	case GPUMODE_POLY_FT4: gpu_poly_ft4(data); break;
	case GPUMODE_POLY_G3:  gpu_poly_g3(data);  break;
	case GPUMODE_POLY_GT3: gpu_poly_gt3(data); break;
	case GPUMODE_POLY_G4:  gpu_poly_g4(data);  break;
	case GPUMODE_POLY_GT4: gpu_poly_gt4(data); break;

	case GPUMODE_LINE_F2: gpu_line_f2(data); break;
	case GPUMODE_LINE_FX: gpu_line_fx(data); break;
	case GPUMODE_LINE_G2: gpu_line_g2(data); break;
	case GPUMODE_LINE_GX: gpu_line_gx(data); break;

	case GPUMODE_TILE:     gpu_tile(data);     break;
	case GPUMODE_SPRITE:   gpu_sprite(data);   break;
	case GPUMODE_TILE_N:   gpu_tile_n(data);   break;
	case GPUMODE_SPRITE_N: gpu_sprite_n(data); break;

	case GPUMODE_COPY_VRAM_VRAM: copy_vram_vram(data); break;
	case GPUMODE_COPY_CPU_VRAM:  copy_cpu_vram(data); break;
	case GPUMODE_COPY_VRAM_CPU:  copy_vram_cpu(data); break;
	}

	gpu.count++;
}

static
void gpu_status(u32 data)
{
	u8 cmd = (u8)((data >> 24) & 0x3F);

	LOG_PRINTF(message[message_idx++], "GPUSTATUS %08X\n", data);
	
	switch (cmd)
	{
	case 0x00: //Reset GPU
		gpu.status_reg |= (1 << 14);

		gpu_status(0x01000000);
		gpu_status(0x02000000);
		gpu_status(0x03000001);
		gpu_status(0x04000000);
		gpu_status(0x05000000);
		gpu_status(0x06C00200);
		gpu_status(0x07040010);
		gpu_status(0x08000000);

		gpu_command(0xE1000000);
		gpu_command(0xE2000000);
		gpu_command(0xE3000000);
		gpu_command(0xE4000000);
		gpu_command(0xE5000000);
		gpu_command(0xE6000000);

		gpu.status_reg |= (1 << 26);
		gpu.status_reg |= (1 << 28);

		//gpu.status_reg = 0x14802000;
		break;

	case 0x01: //Reset Command Buffer
		gpu.status_reg |= (1 << 28);
		set_gpu_mode(GPUMODE_COMMAND);
		break;

	case 0x02: 
		gpu.status_reg &= ~(1 << 24);
		break;

	case 0x03: //Display Disable
		gpu.status_reg &= ~(1 << 23);
		gpu.status_reg |= (data & 1) << 23;
		break;

	case 0x04: //DMA Direction / Data Request
		gpu.status_reg &= ~(3 << 29);
		gpu.status_reg |= (data & 3) << 29;
		break;

	case 0x05: //Start of Display area (in VRAM)
		break;

	case 0x06: //Horizontal Display range (on Screen)
		break;

	case 0x07: //Vertical Display range (on Screen)
		break;

	case 0x08: //Display mode
		gpu.status_reg &= ~(0x7F4000);
		gpu.status_reg |= (data & 0x3F) << 17 | (data & 0x40) << 10 | (data & 0x80) << 7;
		break;

	case 0x09: //Texture Disable
		gpu.allow_texdisable = data & 1;
		break;

	case 0x0B: //Unknown/Internal?
		break;

	case 0x10: case 0x11: case 0x12: case 0x13:
	case 0x14: case 0x15: case 0x16: case 0x17:
	case 0x18: case 0x19: case 0x1A: case 0x1B:
	case 0x1C: case 0x1D: case 0x1E: case 0x1F:
		switch (data & 0xF)
		{
		case 0x00: gpu.return_data = 00; return; // ??
		case 0x01: gpu.return_data = 00; return; // ??
		case 0x02: gpu.return_data = (gpu.return_data & 0x0FFFFF) | gpu.texture_window; return;
		case 0x03: gpu.return_data = (gpu.return_data & 0x0FFFFF) | gpu.draw_area_top_left; return;
		case 0x04: gpu.return_data = (gpu.return_data & 0x0FFFFF) | gpu.draw_area_bot_right; return;
		case 0x05: gpu.return_data = (gpu.return_data & 0x3FFFFF) | gpu.draw_offset; return;
		case 0x06: gpu.return_data = 00; return; // ??
		case 0x07: gpu.return_data = 0x02; return;
		case 0x08: gpu.return_data = 0x00; return;
		case 0x09: gpu.return_data = 00; return; // ??
		case 0x0A: gpu.return_data = 00; return; // ??
		case 0x0B: gpu.return_data = 00; return; // ??
		case 0x0C: gpu.return_data = 00; return; // ??
		case 0x0D: gpu.return_data = 00; return; // ??
		case 0x0E: gpu.return_data = 00; return; // ??
		case 0x0F: gpu.return_data = 00; return; // ??
		}
		break;
	}
}
