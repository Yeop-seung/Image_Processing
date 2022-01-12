
/**************************************************************************
 *
 *	XVID MPEG-4 VIDEO CODEC
 *	colorspace conversions
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *************************************************************************/

 /**************************************************************************
  *
  *	History:
  *
  *	14.04.2002	added user_to_yuv_c()
  *	30.02.2002	out_yuv dst_stride2 fix
  *	26.02.2002	rgb555, rgb565
  *	24.11.2001	accuracy improvement to yuyv/vyuy conversion
  *	28.10.2001	total rewrite <pross@cs.rmit.edu.au>
  *
  **************************************************************************/

#include <stdint.h>


#define MIN(A,B)	((A)<(B)?(A):(B))
#define MAX(A,B)	((A)>(B)?(A):(B))

#define SCALEBITS_IN	8

int32_t RGB_Y_tab[256];
int32_t B_U_tab[256];
int32_t G_U_tab[256];
int32_t G_V_tab[256];
int32_t R_V_tab[256];


/*	yuv -> rgb def's */

#define RGB_Y_OUT		1.164
#define B_U_OUT			2.018
#define Y_ADD_OUT		16

#define G_U_OUT			0.391
#define G_V_OUT			0.813
#define U_ADD_OUT		128

#define R_V_OUT			1.596
#define V_ADD_OUT		128


#define SCALEBITS_OUT	13
#define FIX_OUT(x)		((uint16_t) ((x) * (1L<<SCALEBITS_OUT) + 0.5))

/* initialize rgb lookup tables */

void
colorspace_init(void)
{
	int32_t i;

	for (i = 0; i < 256; i++) {
		RGB_Y_tab[i] = FIX_OUT(RGB_Y_OUT) * (i - Y_ADD_OUT);
		B_U_tab[i] = FIX_OUT(B_U_OUT) * (i - U_ADD_OUT);
		G_U_tab[i] = FIX_OUT(G_U_OUT) * (i - U_ADD_OUT);
		G_V_tab[i] = FIX_OUT(G_V_OUT) * (i - V_ADD_OUT);
		R_V_tab[i] = FIX_OUT(R_V_OUT) * (i - V_ADD_OUT);
	}
}

#define SIZE 4
void yv12_to_rgb32_c(uint8_t* dst,
	int dst_stride,
	uint8_t* y_src,
	uint8_t* u_src,
	uint8_t* v_src,
	int y_stride,
	int uv_stride,
	int width,
	int height)
{
	const uint8_t a = 0;		/* alpha = 0 */
	const uint32_t dst_dif = 2 * dst_stride - (SIZE)*width;
	int32_t y_dif = 2 * y_stride - width;
	uint8_t* dst2 = dst + dst_stride;
	uint8_t* y_src2 = y_src + y_stride;
	uint32_t x, y;
	colorspace_init();

	if (height < 0) {			/* flip image? */
		height = -height;
		y_src += (height - 1) * y_stride;
		y_src2 = y_src - y_stride;
		u_src += (height / 2 - 1) * uv_stride;
		v_src += (height / 2 - 1) * uv_stride;
		y_dif = -width - 2 * y_stride;
		uv_stride = -uv_stride;
	}
	/* process one 2x2 block per iteration */
	for (y = height / 2; y; y--) {
		for (x = 0; x < (uint32_t)width / 2; x++) {
			int u, v;
			int b_u, g_uv, r_v, rgb_y;
			int r, g, b;

			u = u_src[x];
			v = v_src[x];
			b_u = B_U_tab[u];
			g_uv = G_U_tab[u] + G_V_tab[v];
			r_v = R_V_tab[v];

			rgb_y = RGB_Y_tab[*y_src];
			b = MAX(0, MIN(255, (rgb_y + b_u) >> SCALEBITS_OUT));
			g = MAX(0, MIN(255, (rgb_y - g_uv) >> SCALEBITS_OUT));
			r = MAX(0, MIN(255, (rgb_y + r_v) >> SCALEBITS_OUT));
			dst[0] = (b);
			dst[1] = (g);
			dst[2] = (r);
			if ((SIZE) > 3) dst[3] = (a);
			y_src++;

			rgb_y = RGB_Y_tab[*y_src];
			b = MAX(0, MIN(255, (rgb_y + b_u) >> SCALEBITS_OUT));
			g = MAX(0, MIN(255, (rgb_y - g_uv) >> SCALEBITS_OUT));
			r = MAX(0, MIN(255, (rgb_y + r_v) >> SCALEBITS_OUT));
			dst[(SIZE)+0] = (b);
			dst[(SIZE)+1] = (g);
			dst[(SIZE)+2] = (r);
			if ((SIZE) > 3) dst[(SIZE)+3] = (a);
			y_src++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = MAX(0, MIN(255, (rgb_y + b_u) >> SCALEBITS_OUT));
			g = MAX(0, MIN(255, (rgb_y - g_uv) >> SCALEBITS_OUT));
			r = MAX(0, MIN(255, (rgb_y + r_v) >> SCALEBITS_OUT));
			dst2[0] = (b);
			dst2[1] = (g);
			dst2[2] = (r);
			if ((SIZE) > 3) dst2[3] = (a);
			y_src2++;

			rgb_y = RGB_Y_tab[*y_src2];
			b = MAX(0, MIN(255, (rgb_y + b_u) >> SCALEBITS_OUT));
			g = MAX(0, MIN(255, (rgb_y - g_uv) >> SCALEBITS_OUT));
			r = MAX(0, MIN(255, (rgb_y + r_v) >> SCALEBITS_OUT));
			dst2[(SIZE)+0] = (b);
			dst2[(SIZE)+1] = (g);
			dst2[(SIZE)+2] = (r);
			if ((SIZE) > 3) dst2[(SIZE)+3] = (a);
			y_src2++;

			dst += 2 * (SIZE);
			dst2 += 2 * (SIZE);
		}
		dst += dst_dif;
		dst2 += dst_dif;
		y_src += y_dif;
		y_src2 += y_dif;
		u_src += uv_stride;
		v_src += uv_stride;
	}
}
