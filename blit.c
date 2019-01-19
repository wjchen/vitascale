/*
	PSP VSH 24bpp text bliter
*/
#include "blit.h"
#include <psp2/paf.h>
#include <psp2/kernel/sysmem.h>

#define MIN(x, y) (x) < (y)?(x):(y)
#define MAX(x, y) (x) > (y)?(x):(y)

extern unsigned char msx[];

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static int pwidth, pheight, bufferwidth, pixelformat;
static unsigned int* vram32;

static uint32_t fcolor = 0x00ffffff;
static uint32_t bcolor = 0xff000000;

typedef struct {
	char  gameid[16];
	int   sx;	  // src pic pos
	int   sy;	  
	int   swidth;  // src pic width
	int   sheight; // src pic height
	float scale;

	int   dx;
	int   dy;
	int   dwidth;
	int   dheight;

	int   pwidth;  // pic width
	int   pheight; // pic height
} scale_opt_t;


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static uint32_t adjust_alpha(uint32_t col)
{
	uint32_t alpha = col>>24;
	uint8_t mul;
	uint32_t c1,c2;

	if(alpha==0)	return col;
	if(alpha==0xff) return col;

	c1 = col & 0x00ff00ff;
	c2 = col & 0x0000ff00;
	mul = (uint8_t)(255-alpha);
	c1 = ((c1*mul)>>8)&0x00ff00ff;
	c2 = ((c2*mul)>>8)&0x0000ff00;
	return (alpha<<24)|c1|c2;
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

int blit_set_frame_buf(const SceDisplayFrameBuf *param)
{

	pwidth = param->width;
	pheight = param->height;
	vram32 = param->base;
	bufferwidth = param->pitch;
	pixelformat = param->pixelformat;

	if( (bufferwidth==0) || (pixelformat!=0)) return -1;

	fcolor = 0x00ffffff;
	bcolor = 0xff000000;

	return 0;
}

void blit_set_color(int fg_col,int bg_col)
{
	fcolor = fg_col;
	bcolor = bg_col;
}

/////////////////////////////////////////////////////////////////////////////
// blit text
/////////////////////////////////////////////////////////////////////////////
int blit_string(int sx,int sy,const char *msg)
{
	int x,y,p;
	int offset;
	char code;
	unsigned char font;
	uint32_t fg_col,bg_col;

	uint32_t col,c1,c2;
	uint32_t alpha;

	fg_col = adjust_alpha(fcolor);
	bg_col = adjust_alpha(bcolor);


	if( (bufferwidth==0) || (pixelformat!=0)) return -1;

	for(x=0;msg[x] && x<(pwidth/16);x++)
	{
		code = msg[x] & 0x7f; // 7bit ANK
		for(y=0;y<8;y++)
		{
			offset = (sy+(y*2))*bufferwidth + sx+x*16;
			font = y>=7 ? 0x00 : msx[ code*8 + y ];
			for(p=0;p<8;p++)
			{
				col = (font & 0x80) ? fg_col : bg_col;
				alpha = col>>24;
				if(alpha==0)
				{
					vram32[offset] = col;
					vram32[offset + 1] = col;
					vram32[offset + bufferwidth] = col;
					vram32[offset + bufferwidth + 1] = col;
				}
				else if(alpha!=0xff)
				{
					c2 = vram32[offset];
					c1 = c2 & 0x00ff00ff;
					c2 = c2 & 0x0000ff00;
					c1 = ((c1*alpha)>>8)&0x00ff00ff;
					c2 = ((c2*alpha)>>8)&0x0000ff00;
					uint32_t color = (col&0xffffff) + c1 + c2;
					vram32[offset] = color;
					vram32[offset + 1] = color;
					vram32[offset + bufferwidth] = color;
					vram32[offset + bufferwidth + 1] = color;
				}
				font <<= 1;
				offset+=2;
			}
		}
	}
	return x;
}

int blit_string_ctr(int sy,const char *msg)
{
	int sx = 960/2 - strlen(msg)*(16/2);
	return blit_string(sx,sy,msg);
}

int blit_stringf(int sx, int sy, const char *msg, ...)
{
	va_list list;
	char string[512];

	va_start(list, msg);
	vsprintf(string, msg, list);
	va_end(list);

	return blit_string(sx, sy, string);
}


void draw_rectangle(uint32_t  x, uint32_t  y, uint32_t  w, uint32_t h, uint32_t  inColor)
{
	int i, j;
	uint32_t  c1,c2;
	uint32_t  in_col = adjust_alpha(inColor);
	uint32_t  alpha = in_col>>24;
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			c2 = vram32[(x + j) + (y + i)*bufferwidth];
			c1 = c2 & 0x00ff00ff;
			c2 = c2 & 0x0000ff00;
			c1 = ((c1*alpha)>>8)&0x00ff00ff;
			c2 = ((c2*alpha)>>8)&0x0000ff00;
			uint32_t  color = (in_col&0xffffff) + c1 + c2;
			((int *)vram32)[(x + j) + (y + i)*bufferwidth] = color;
		}
	}
}

static void bgra_scale(scale_opt_t *opt)
{
	int x, y;
	int iscale = 256.0f/opt->scale;
	if (iscale == 256) {
		return;
	}
	int width = opt->swidth;
	int height = opt->sheight;
	int widthDst = opt->dwidth;
	int heightDst = opt->dheight;
	int maxy = MIN(heightDst, opt->pheight-opt->dy);
	int maxx = MIN(widthDst, opt->pwidth-opt->dx);
	// make sure (((x * iscale) >> 8) <= width-1);
	maxx = MIN(maxx, (width-1)*256/iscale+1);
	for (y = maxy-1; y >= 1; y--) {
		int y1 = MIN((y * iscale) >> 8, height-1);
		for (x = 0; x < maxx; x++) {
			int x1 = (x*iscale) >> 8;
			vram32[(opt->dy+y)*bufferwidth + (opt->dx+x)] = vram32[(y1+opt->sy)*bufferwidth + opt->sx+x1];
		}
	}
}

static int scale_opt_create(scale_opt_t *opt, int sx, int sy, int swidth, int sheight, float scale)
{
	if (opt == NULL) {
		return -1;
	}
	sx = MIN(MAX(sx, 0), pwidth-1);
	sy = MIN(MAX(sy, 0), pheight-1);
	swidth = MIN(MAX(swidth, 0), pwidth);
	sheight = MIN(MAX(sheight, 0), pheight);

	// src
	opt->sx = sx;
	opt->sy = sy;
	opt->swidth = swidth;
	opt->sheight = sheight;
	// fix src boundary
	if (opt->sx+opt->swidth > pwidth) {
		opt->swidth = pwidth - opt->sx;
	}
	if (opt->sy + opt->sheight > pheight) {
		opt->sheight = pheight - opt->sy;
	}
	if (opt->swidth <= 0 || opt->sheight <= 0) {
		return -1;
	}
	opt->scale = scale;
	// buf
	opt->pwidth = pwidth;
	opt->pheight = pheight;

	// dst
	opt->dwidth = opt->swidth*opt->scale;
	opt->dheight = opt->sheight*opt->scale;
	opt->dwidth = MIN(opt->dwidth, opt->pwidth);
	opt->dheight = MIN(opt->dheight, opt->pheight);

	opt->dx = MAX(opt->sx - (opt->dwidth - opt->swidth)/2, 0);
	opt->dy = opt->sy;
	if (opt->dx+opt->dwidth > opt->pwidth) {
		opt->dwidth = opt->pwidth - opt->dx;
	}
	if (opt->dy + opt->dheight > pheight) {
		opt->dheight = pheight - opt->dy;
	}
	if (opt->dwidth <= 0 || opt->dheight <= 0) {
		return -1;
	}
	return 0;
}

void bilt_scale_rect(int sx, int sy, int swidth, int sheight, float scale)
{
	scale_opt_t opt;
	if( (bufferwidth==0) || (pixelformat!=0)) return;
	if (scale <= 1.0f) return;
	int ret = scale_opt_create(&opt, sx, sy, swidth, sheight, scale);
	if (ret == 0) {
		bgra_scale(&opt);
	}
}
