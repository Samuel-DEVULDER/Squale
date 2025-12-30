#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <float.h>
#include <sys/time.h>
#include <math.h>

#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"
#include "stb/stb_ds.h"

#define PRIVATE static

PRIVATE uint8_t exo = 0, bayer = 0, verbose = 0;
PRIVATE char *input_file, *output_file;

typedef float vec3[3];
typedef float vec4[4];

typedef struct color {
	vec3	pt;
	float   weight;
	float   intens;
	uint8_t	index;
} color;

typedef struct tetra {
	color *p[4];
	vec3 n012, n023, n031, n132;
	struct tetra *prev, *next;
} tetra;

#define FULL	1.0f
#define HALF	0.542574f

PRIVATE color palette[15];

PRIVATE int color_cmp_by_intens(const void *pa, const void *pb) {
	const color *a = pa, *b = pb;
	return a->intens < b->intens ? -1 :
	       a->intens > b->intens ? +1 : 0;	
}

PRIVATE int color_cmp_by_weight(const void *pa, const void *pb) {
	const color *a = pa, *b = pb;
	return a->weight > b->weight ? -1 :
	       a->weight < b->weight ? +1 : 0;	
}

PRIVATE void set_palette(int i, float r, float g, float b) {
	color *c = &palette[i];
	c->index = i;
	c->pt[0] = r;
	c->pt[1] = g;
	c->pt[2] = b;
	c->intens = .3f*r +.59f*g  + .11f*b;
}

PRIVATE tetra tetras[28], *tetra_list;

PRIVATE vec3 *vec3_madd(vec3 *t, vec3 *u, float k, vec3 *v) {
	int i;
	for(i=0; i<3; ++i) (*t)[i] = (*u)[i]+k*(*v)[i];
	return t;
}

#define vec3_add(t, u, v) vec3_madd((t),(u),+1.0f,(v))
#define vec3_sub(t, u, v) vec3_madd((t),(u),-1.0f,(v))

PRIVATE vec3 *vec3_mul(vec3 *t, vec3 *u, vec3 *v) {
	(*t)[0] = (*u)[1]*(*v)[2] - (*u)[2]*(*v)[1];
	(*t)[1] = (*u)[2]*(*v)[0] - (*u)[0]*(*v)[2];
	(*t)[2] = (*u)[0]*(*v)[1] - (*u)[1]*(*v)[0];
	return t;
}

PRIVATE float vec3_dot(vec3 *u, vec3 *v) {
	return (*u)[0]*(*v)[0] + (*u)[1]*(*v)[1] + (*u)[2]*(*v)[2];
}

PRIVATE void new_tetra(tetra *tetra, int a, int b, int c, int d) {
	vec3 p01,p02,p03,p12,p13;

	tetra->p[0] = &palette[a];
	tetra->p[1] = &palette[b];
	tetra->p[2] = &palette[c];
	tetra->p[3] = &palette[d];
			
	tetra->prev = NULL;
	tetra->next = tetra_list;
	if(tetra->next) tetra->next->prev = tetra;
	
	vec3_sub(&p01, &tetra->p[1]->pt, &tetra->p[0]->pt);
	vec3_sub(&p02, &tetra->p[2]->pt, &tetra->p[0]->pt);
	vec3_sub(&p03, &tetra->p[3]->pt, &tetra->p[0]->pt);
	vec3_sub(&p12, &tetra->p[2]->pt, &tetra->p[1]->pt);
	vec3_sub(&p13, &tetra->p[3]->pt, &tetra->p[1]->pt);
	
	vec3_mul(&tetra->n012, &p01, &p02);
	vec3_mul(&tetra->n023, &p02, &p03);
	vec3_mul(&tetra->n031, &p03, &p01);
	vec3_mul(&tetra->n023, &p02, &p03);
	vec3_mul(&tetra->n132, &p13, &p03);
	
	assert(vec3_dot(&tetra->n012, &p03)>=0);
}

/* https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf */
PRIVATE void tetra_proj3(float *w0, float *w1, float *w2, 
                         vec3 *point, vec3 *v0, vec3 *v1, vec3 *v2) {
	vec3 diff, edge0, edge1;
	float a00,a01,a11,b0,b1,det,t0,t1;
	
	vec3_sub(&diff,  point, v0);
	vec3_sub(&edge0, v1, v0);
	vec3_sub(&edge1, v2, v0);
	
	a00 = vec3_dot(&edge0, &edge0);
	a01 = vec3_dot(&edge0, &edge1);
	a11 = vec3_dot(&edge1, &edge1);
	
	b0 = -vec3_dot(&diff, &edge0);
	b1 = -vec3_dot(&diff, &edge1);
	
	det = a00 * a11 - a01 * a01;
	t0  = a01 * b1  - a11 * b0;
	t1  = a01 * b0  - a00 * b1;
	
	if(t0 + t1 <= det) {
		if(t0 < 0) {
			if(t1 < 0) { /* region 4 */
				if(b0 < 0) {
					t0 = -b0>=a00 ? 1 : -b0/a00;
					t1 = 0;
				} else {
					t0 = 0;
					t1 = b1>=0 ? 0 : -b1>=a11 ? 1 : -b1/a11;
				}
			} else { /* region 3 */
				t0 = 0;
				t1 = b1>=0 ? 0 : -b1>=a11 ? 1 : -b1/a11;
			}
		} else if(t1 < 0) { /* region 5 */
			t0 = b0>=0 ? 0 : -b0>=a00 ? 1 : -b0/a00;
			t1 = 0;
		} else { /* region 0, interior */
			t0 /= det;
			t1 /= det;
		}
	} else {
		float tmp0, tmp1, numer, denom;
		if(t0 < 0) { /* region 2 */
			tmp0 = a01 + b0;
			tmp1 = a11 + b1;
			if(tmp1 > tmp0) {
				numer = tmp1 - tmp0;
				denom = a00 - a01 - a01 + a11;
				t0 = numer>=denom ? 1 : numer/denom;
				t1 = 1 - t0;
			} else {
				t0 = 0;
				t1 = tmp1<=0 ? 1 : b1>=0 ? 0 : -b1/a11;
			}
		} else if(t1 < 0) { /* region 6 */
			tmp0 = a01 + b1;
			tmp1 = a00 + b0;
			if(tmp1 > tmp0) {
				numer = tmp1 - tmp0;
				denom = a00 - a01 - a01 + a11;
				t1 = numer>=denom ? 1 : numer/denom;
				t0 = 1 - t1;
			} else {
				t1 = 0;
				t0 = tmp1<=0 ? 1 : b0>=0 ? 0 : -b0/a00;
			}
		} else { /* region 1 */
			numer = a11 + b1  - a01 - b0;
			denom = a00 - a01 - a01 + a11;
			t0 = numer<=0 ? 0 : numer>=denom ? 1 : numer/denom;
			t1 = 1 - t0;
		}
	}
	*w0 = 1-t0-t1; *w1 = t0; *w2 = t1;
}

PRIVATE void tetra_proj2(float *w0, float *w1, vec3 *point, vec3 *v0, vec3 *v1) {
	vec3 v10, q;
	float t;
	
	vec3_sub(&v10, v1, v0);
	t = vec3_dot(&v10, &v10);
	
	if(t<=0) {
		t = 0;
	} else {
		vec3_sub(&q, point, v0);
		t = vec3_dot(&q,&v10)/t;
		t = t<=0 ? 0 : t>1 ? 1 : t;
	}
	*w0 = 1-t; *w1 = t;
}

PRIVATE void tetra_coord(tetra *tetra, vec3 *p, vec3 *b) {
	color **T = tetra->p; 
	float w0,w1,w2,w3;
	
	do {	vec3 q;

		vec3_sub(&q, p, &T[0]->pt);
		w3 = vec3_dot(&q, &tetra->n012);
		w1 = vec3_dot(&q, &tetra->n023);
		w2 = vec3_dot(&q, &tetra->n031);
		
		vec3_sub(&q, p, &T[1]->pt);
		w0 = vec3_dot(&q, &tetra->n132);
	} while(0);
	
	if(w0 < 0) {
		if(w1 < 0) {
			if(w2 < 0) {
				w3 = 1;
				w0 = w1 = w2 = 0;
			} else if(w3 < 0) {
				w2 = 1;
				w0 = w1 = w3 = 0;
			} else {
				tetra_proj2(&w2, &w3, p, &T[2]->pt, &T[3]->pt);
				w0 = w1 = 0;
			}
		} else if(w2 < 0) {
			if(w3 < 0) {
				w1 = 1;
				w0 = w2 = w3 = 0;
			} else {
				tetra_proj2(&w1, &w3, p, &T[1]->pt, &T[3]->pt);
				w0 = w2 = 0;
			}
		} else if(w3 < 0) {
			tetra_proj2(&w1, &w2, p, &T[1]->pt, &T[2]->pt);
			w0 = w3 = 0;
		} else {
			tetra_proj3(&w1, &w2, &w3, p, &T[1]->pt, &T[2]->pt, &T[3]->pt);
			w0 = 0;
		}
	} else if(w1 < 0) {
		if(w2 < 0) {
			if(w3 < 0) {
				w0 = 1;
				w1 = w2 = w3 = 0;
			} else {
				tetra_proj2(&w0, &w3, p, &T[0]->pt, &T[3]->pt);
				w1 = w2 = 0;
			}
		} else if(w3 < 0) {
			tetra_proj2(&w0, &w2, p, &T[0]->pt, &T[2]->pt);
			w1 = w3 = 0;
		} else {
			tetra_proj3(&w0, &w2, &w3, p, &T[0]->pt, &T[2]->pt, &T[3]->pt);
			w1 = 0;
		}
	} else if(w2 < 0) {
		if(w3 < 0) {
			tetra_proj2(&w0, &w1, p, &T[0]->pt, &T[1]->pt);
			w2 = w3 = 0;
		} else {
			tetra_proj3(&w0, &w1, &w3, p, &T[0]->pt, &T[1]->pt, &T[3]->pt);
			w2 = 0;
		}
	} else {
		tetra_proj3(&w0, &w1, &w2, p, &T[0]->pt, &T[1]->pt, &T[2]->pt);
		w3 = 0;
	}

	do { 	float  tot = w0 + w1 + w2 + w3;
		tot = tot<=0 ? 0 : 1.0f/tot;
	
		w0 *= tot; w1 *= tot; w2 *= tot; w3 *= tot;
	} while(0);
	
	tetra->p[0]->weight = w0;
	tetra->p[1]->weight = w1;
	tetra->p[2]->weight = w2;
	tetra->p[3]->weight = w3;
	
	if(b != NULL) {
		int i;
		for(i=0;i<3;++i) {
			(*b)[i] = w0*T[0]->pt[i] + w1*T[1]->pt[i]
		                + w2*T[2]->pt[i] + w3*T[3]->pt[i];
		}
	}
}

uint8_t dith_vac[8][8] = {
	{40,61,2,39,19,43,23,8},
	{12,20,32,49,58,13,51,56},
	{29,46,53,9,27,33,1,36},
	{60,5,24,42,17,62,45,18},
	{41,15,63,37,4,54,10,25},
	{57,7,30,50,21,28,38,52},
	{22,35,44,11,59,47,14,3},
	{48,16,55,26,6,34,64,31}
};

uint8_t dith_bayer[8][8] = {
	{ 1,33, 9,41, 3,35,11,43},
	{49,17,57,25,51,19,59,27},
	{13,45, 5,37,15,47, 7,39},
	{61,29,53,21,63,31,55,23},
	{ 4,36,12,44, 2,34,10,42},
	{52,20,60,28,50,18,58,26},
	{16,48, 8,40,14,46, 6,38},
	{64,32,56,24,62,30,54,22}
};

PRIVATE uint32_t dith_key(vec3 *p) {
	return  (uint32_t)(24*powf((*p)[0]<0?0:(*p)[0],0.45f)) +
		(uint32_t)(24*powf((*p)[1]<0?0:(*p)[1],0.45f))*32 +
		(uint32_t)(24*powf((*p)[2]<0?0:(*p)[2],0.45f))*32*32;
}

PRIVATE struct dith_cache {
	uint32_t key;
	uint8_t  value[64];
} *dith_cache;

PRIVATE tetra *dith_find_tetra(vec3 *p) {
	float w0 = 0, w1 = 0, w2 = 0, w3 = 0;
	float  best_d = FLT_MAX;
	tetra *best_t = NULL, *t;
	
	for(t = tetra_list; t; t = t->next) {
		float d; vec3 q;
	
		tetra_coord(t, p, &q);
		d = vec3_dot(&q, &q);
		
		if(d < best_d) {
			best_d = d;
			best_t = t;
			w0 = t->p[0]->weight;
			w1 = t->p[1]->weight;
			w2 = t->p[2]->weight;
			w3 = t->p[3]->weight;
			if(d <= 1e-5) break; /* shortcut */
		}
	}
	
	/* move found to first place.
	   idea here is to have an LRU organisation
	 */
	if(best_t != tetra_list) {
		best_t->next->prev = best_t->prev;
		best_t->prev->next = best_t->next;
		
		best_t->next = tetra_list;
		best_t->prev = NULL;
		
		tetra_list = best_t;
	}
	
	best_t->p[0]->weight = w0;
	best_t->p[1]->weight = w1;
	best_t->p[2]->weight = w2;
	best_t->p[3]->weight = w3;
		
	return best_t;
}

#define lenof(array) (sizeof(array)/sizeof(array[0]))

PRIVATE uint8_t dith(const uint8_t *dith, const int mx, const int my,  
		     const int x, const int y, vec3 *p) {
	const uint32_t key = dith_key(p);
	struct dith_cache *cache = hmgetp_null(dith_cache, key);
	
	const int mxy = mx*my;
	assert(mxy <= lenof(cache->value));

	if(cache == NULL) {
		tetra *t = dith_find_tetra(p);
		color *sel[4] = {t->p[0], t->p[1], t->p[2], t->p[3]},
		      *tab[lenof(cache->value)];
		
		qsort(sel, 4, sizeof(sel[0]), color_cmp_by_weight);
		
		do {
			int i = 0; float m = 0;			
			m += sel[0]->weight * mxy; while(i<m) tab[i++] = sel[0];
			m += sel[1]->weight * mxy; while(i<m) tab[i++] = sel[1];
			m += sel[2]->weight * mxy; while(i<m) tab[i++] = sel[2];
			while(i<mxy) tab[i++] = sel[3];
		} while(0);
		
		qsort(tab, mxy, sizeof(tab[0]), color_cmp_by_intens);
		
		do {
			struct dith_cache new_entry; int i;
			new_entry.key = key;
			for(i=0; i<mxy; ++i) new_entry.value[i] = tab[i]->index;
			hmputs(dith_cache, new_entry);
		} while(0);
		
		cache = hmgetp_null(dith_cache, key);
		assert(cache != NULL);
	}
	
	return cache->value[dith[(x % mx)*my + (y % my)]-1];
}

typedef struct {
	int w;
	int h;
	uint8_t *sRGB;
	uint8_t bitmap[65536];
	struct timeval start;
} pic;

PRIVATE void pic_load(pic *pic, const char *filename) {
	int n;
	
	gettimeofday(&pic->start, NULL);
	
	if(!stbi_info(filename, &pic->w, &pic->h, &n)) {
		fprintf(stderr, "Unsupported image: %s\n", filename);
		exit(-1);
	}
	if(verbose) {
		printf("%s (%d x %d x %d)...", filename, pic->w, pic->h, n);
		fflush(stdout);
	}
	pic->sRGB = stbi_load(filename, &pic->w, &pic->h, &n, 3);
	if(pic->sRGB == NULL)  {
		if(verbose) printf("error\n");
		else fprintf(stderr, "Error while loading: %s\n", filename);
		exit(-1);
	}	
}

PRIVATE void pic_save(pic *pic, const char *filename) {
	FILE *f = fopen(filename, "wb");
	
	if(f==NULL) {
		perror(filename);
		return;
	}
	
	fprintf(f, exo ? "SQP\2" : "SQP\1");
	
	if(exo) {
		
	} else {
		int i;
		for(i=0; i<65536; i+=2) {
			fputc(pic->bitmap[i]*16+pic->bitmap[i+1], f);
		}
	}
	fflush(f);
	fclose(f);
	
	if(verbose) {
		struct timeval stop;
		float us;
		
		gettimeofday(&stop, NULL);
		us = (stop.tv_sec  - pic->start.tv_sec) * 1000000.0f + 
		      stop.tv_usec - pic->start.tv_usec;
		printf("done (%.1fms)\n", us/1000.0);
	}
}

PRIVATE vec3 *pic_get_linear_color(pic *pic, int x, int y, vec3 *ret) {
	static float sRGB[256];
	
	if(sRGB[255]==0) {
		int i;
		for(i=0;i<256;++i) {
			float x = i/255;
			sRGB[i] = x<=0.04045f ? x/12.92f : powf((x+0.055f)/1.055f, 2.4f);
		}
	}
	
	if(x<0 || y<0 || x>=pic->w || y>=pic->h) {
		(*ret)[0] = (*ret)[1] = (*ret)[2] = 0;
	} else {
		uint8_t *img = pic->sRGB + 3*(pic->w*y + x);
		(*ret)[0] = sRGB[img[0]];
		(*ret)[1] = sRGB[img[1]];
		(*ret)[2] = sRGB[img[2]];
	}
	
	return ret;
}

PRIVATE void squale_coord(pic *pic, int x, int y, int *rx, int *ry) {
	int w=pic->w, h=pic->h;
	if(w<=h) {
		*rx = (x*h + 128*(w - h))/256;
		*ry = (y*h)/256;
	} else {
		*rx = (x*w)/256;
		*ry = (y*w + 128*(h - w))/256;
	}
}

PRIVATE vec3 *squale_color(pic *pic, int x, int y, vec3 *ret) {
	int x1, y1, x2, y2, i, j;
	float k; vec3 p;
	
	squale_coord(pic, x-1,y-1, &x1, &y1);
	squale_coord(pic, x+1,y+1, &x2, &y2);
	
	k = 1.0f/(1.0f+y2-y1)*(1.0f+x2-x1);
	
	(*ret)[0] = (*ret)[1] = (*ret)[2] = 0;
	for(i=x1; i<=x2; ++i) for(j = y1; j<=y2; ++j) {
		vec3_madd(ret, ret, k, pic_get_linear_color(pic, i,j, &p));
	}
	
	return ret;
}

PRIVATE void pic_dither(pic *pic, int x, int y) {
	vec3 p; 
	
	int8_t c = dith(
	        bayer ? &dith_bayer[0][0]    : &dith_vac[0][0], 
		bayer ? lenof(dith_bayer)    : lenof(dith_vac),
		bayer ? lenof(dith_bayer[0]) : lenof(dith_vac[0]),
		x, y, squale_color(pic, x, y, &p)
	);
	
	pic->bitmap[x + y*256] = c;
}

PRIVATE void pic_conv_h(pic *pic) {
	static int dir[] = {256,1,-256,-1}, a, p, l = 8, b = 1;
	if(pic==NULL)  {
		l = 8;
		b = 1;
	} else if(l == 0) {
		pic_dither(pic, p & 255, p>>8);
	} else {
		--l;
		a -= (b=-b); pic_conv_h(pic);
		p = p + dir[a&3]; 
		a -= (b=-b); pic_conv_h(pic);
		p = p + dir[a&3]; pic_conv_h(pic);
		p = p + dir[a&3]; pic_conv_h(pic);
		a += (b=-b); 
		p = p + dir[a&3]; pic_conv_h(pic); 
		a += (b=-b);		
		++l;
	}
}

PRIVATE void pic_conv_l(pic *pic) {
	int i;
	for(i=0;i<65536;++i) pic_dither(pic, i & 255, i>>8);
}

PRIVATE void init(void) {
	stbds_rand_seed(time(0));	

	set_palette( 0, FULL*1, FULL*1, FULL*1);
	set_palette( 1, FULL*1, FULL*1, FULL*0);
	set_palette( 2, FULL*1, FULL*0, FULL*1);
	set_palette( 3, FULL*1, FULL*0, FULL*0);
	set_palette( 4, FULL*0, FULL*1, FULL*1);
	set_palette( 5, FULL*0, FULL*1, FULL*0);
	set_palette( 6, FULL*0, FULL*0, FULL*1);
	set_palette( 7, FULL*0, FULL*0, FULL*0);
	set_palette( 8, HALF*1, HALF*1, HALF*1);
	set_palette( 9, HALF*1, HALF*1, HALF*0);
	set_palette(10, HALF*1, HALF*0, HALF*1);
	set_palette(11, HALF*1, HALF*0, HALF*0);
	set_palette(12, HALF*0, HALF*1, HALF*1);
	set_palette(13, HALF*0, HALF*1, HALF*0);
	set_palette(14, HALF*0, HALF*0, HALF*1);

	new_tetra(&tetras[ 0], 0, 4, 1, 5);
	new_tetra(&tetras[ 1], 0, 2, 4, 6);
	new_tetra(&tetras[ 2], 0, 4, 5, 8);
	new_tetra(&tetras[ 3], 0, 6, 4, 8);
	new_tetra(&tetras[ 4], 0, 1, 3, 8);
	new_tetra(&tetras[ 5], 0, 2, 6, 8);
	new_tetra(&tetras[ 6], 0, 3, 2, 8);
	new_tetra(&tetras[ 7], 0, 5, 1, 8);
	new_tetra(&tetras[ 8], 3, 8, 1, 9);
	new_tetra(&tetras[ 9], 5, 1, 8, 9);
	new_tetra(&tetras[10], 3, 2, 8,10);
	new_tetra(&tetras[11], 6, 8, 2,10);
	new_tetra(&tetras[12], 7, 9, 8,10);
	new_tetra(&tetras[13], 3, 8, 9,10);
	new_tetra(&tetras[14], 3,10, 9,11);
	new_tetra(&tetras[15], 7, 9,10,11);
	new_tetra(&tetras[16], 5, 8, 4,12);
	new_tetra(&tetras[17], 7, 8, 9,12);
	new_tetra(&tetras[18], 7,10, 8,12);
	new_tetra(&tetras[19], 4, 8, 6,12);
	new_tetra(&tetras[20], 5, 8,12,13);
	new_tetra(&tetras[21], 5, 9, 8,13);
	new_tetra(&tetras[22], 7,12, 9,13);
	new_tetra(&tetras[23], 9,12, 8,13);
	new_tetra(&tetras[24], 8,12,10,14);
	new_tetra(&tetras[25], 6, 8,10,14);
	new_tetra(&tetras[26],10,12, 7,14);
	new_tetra(&tetras[27], 6,12, 8,14);
}

PRIVATE void usage(char *av0) {
	printf("Usage: %s [options] <inputimage.ext> -o <outputfile.ext>\n", av0);
	printf("options:\n");
	printf(" -h : prints this help\n");
	printf(" -v : verbose\n");
	printf(" -x : compresses with exomizer\n");
	printf(" -b : bayer dithering\n");
	exit(0);
}

PRIVATE void parse(int ac, char **av) {
	int i;
	for(i=1; i<ac; ++i) {
		if(!strcmp("?", av[i])
		|| !strcmp("-h", av[i])
		|| !strcmp("--help", av[i])
		|| 0) usage(av[0]);
		else if(!strcmp("-x", av[i])) 
			exo = 1;
		else if(!strcmp("-b", av[i])) 
			bayer = 1;
		else if(!strcmp("-o", av[i]) && i<ac-1)
			output_file = av[++i];
		else if(input_file==NULL)
			input_file = av[i];
		else {
			fprintf(stderr, "Unknown argument: %s\n" , av[i]);
			exit(-1);
		}
	}
	
	if(input_file == NULL) {
		fprintf(stderr, "Missing input file\n");
		exit(-1);
	}

	if(input_file == NULL) {
		fprintf(stderr, "Missing output file\n");
		exit(-1);
	}
}

int main(int ac, char **av) {
	parse(ac, av);
	
	init();
	do {
		pic pic;
		
		pic_load(&pic, input_file);
		pic_conv_l(&pic);
		pic_save(&pic, output_file);
	} while(0);
	
	return 0;
}