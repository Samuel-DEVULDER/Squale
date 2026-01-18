#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <float.h>
#include <sys/time.h>
#include <math.h>

#include "stb/stb_ds.h"

#include "stb/stb_image.h"
#include "stb/stb_image_resize2.h"
#include "stb/stb_image_write.h"

#include "membuf_io.h"
#include "exo_helper.h"

#define length_of(array) (sizeof(array)/sizeof(array[0]))

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#define PRIVATE static

#define FULL_INTENSITY	255
//#define HALF_INTENSITY	187
// #define HALF_INTENSITY	157
#define HALF_INTENSITY	127

PRIVATE uint8_t dith_vac[8][8] = {
	{40,61, 2,39,19,43,23, 8},
	{12,20,32,49,58,13,51,56},
	{29,46,53, 9,27,33, 1,36},
	{60, 5,24,42,17,62,45,18},
	{41,15,63,37, 4,54,10,25},
	{57, 7,30,50,21,28,38,52},
	{22,35,44,11,59,47,14, 3},
	{48,16,55,26, 6,34,64,31}
};

/* https://imagemagick.org/source/thresholds.xml */

PRIVATE uint8_t dith_threshold[1][1] = {
	{ 1},
};

PRIVATE uint8_t dith_checks[2][2] = {
	{ 1, 2},
	{ 2, 1},
};

PRIVATE uint8_t dith_o2[2][2] = {
	{ 1, 3},
	{ 4, 2},
};

PRIVATE uint8_t dith_o3[3][3] = {
	{ 3, 7, 4},
	{ 6, 1, 9},
	{ 2, 8, 5}
};

PRIVATE uint8_t dith_o4[4][4] = {
	{ 1, 9, 3,11},
	{13, 5,15, 7},
	{ 4,12, 2,10},
	{16, 8,14, 6},
};

PRIVATE uint8_t dith_o8[8][8] = {
	{ 1,33, 9,41, 3,35,11,43},
	{49,17,57,25,51,19,59,27},
	{13,45, 5,37,15,47, 7,39},
	{61,29,53,21,63,31,55,23},
	{ 4,36,12,44, 2,34,10,42},
	{52,20,60,28,50,18,58,26},
	{16,48, 8,40,14,46, 6,38},
	{64,32,56,24,62,30,54,22}
};

PRIVATE uint8_t dith_h4a[4][4] = {
	{ 4, 2, 7, 5},
	{ 3, 1, 8, 6},
	{ 7, 5, 4, 2},
	{ 8, 6, 3, 1}
};

PRIVATE uint8_t dith_h6a[6][6] = {
	{14,13,10, 8, 2, 3},
	{16,18,12, 7, 1, 4},
	{15,17,11, 9, 6, 5},
	{ 8, 2, 3,14,13,10},
	{ 7, 1, 4,16,18,12},
	{ 9, 6, 5,15,17,11}
};

PRIVATE uint8_t dith_h8a[8][8] = {
	{13, 7, 8,14,17,21,22,18},
	{ 6, 1, 3, 9,28,31,29,23},
	{ 5, 2, 4,10,27,32,30,24},
	{16,12,11,15,20,26,25,19},
	{17,21,22,18,13, 7, 8,14},
	{28,31,29,23, 6, 1, 3, 9},
	{27,32,30,24, 5, 2, 4,10},
	{20,26,25,19,16,12,11,15}
};

PRIVATE uint8_t dith_h4o[4][4] = {
	{ 7,13,11, 4},
	{12,16,14, 8},
	{10,15, 6, 2},
	{ 5, 9, 3, 1}
};

PRIVATE uint8_t dith_h6o[6][6] = {
	{ 7,17,27,14, 9, 4},
	{21,29,33,31,18,11},
	{24,32,36,34,25,22},
	{19,30,35,28,20,10},
	{ 8,15,26,16, 6, 2},
	{ 5,13,23,12, 3, 1} 
};

PRIVATE uint8_t dith_h8o[8][8] = {
	{ 7,21,33,43,36,19, 9, 4},
	{16,27,51,55,49,29,14,11},
	{31,47,57,61,59,45,35,23},
	{41,53,60,64,62,52,40,38},
	{37,44,58,63,56,46,30,22},
	{15,28,48,54,50,26,17,10},
	{ 8,18,34,42,32,20, 6, 2},
	{ 5,13,25,39,24,12, 3, 1}
};

PRIVATE uint8_t dith_c5b[5][5] = {
	{ 1,21,16,15, 4},
	{ 5,17,20,19,14},
	{ 6,21,25,24,12},
	{ 7,18,22,23,11},
	{ 2, 8, 9,10, 3}
};

PRIVATE uint8_t dith_c6b[6][6] = {
	{ 1, 5,14,13,12, 4},
	{ 6,22,28,27,21,11},
	{15,29,35,34,26,20},
	{16,30,36,33,25,19},
	{ 7,23,31,32,24,10},
	{ 2, 8,17,18, 9, 3}
};

PRIVATE uint8_t dith_c7b[7][7] = {
	{ 3, 9,18,28,17, 8, 2},
	{10,24,33,39,32,23, 7},
	{19,34,44,48,43,31,16},
	{25,40,45,49,47,38,27},
	{20,35,41,46,42,29,15},
	{11,21,36,37,28,22, 6},
	{ 4,12,13,26,14, 5, 1}
};

PRIVATE uint8_t dith_c5w[5][5] = {
	{25,21,10,11,22},
	{20, 9, 6, 7,12},
	{19, 5, 1, 2,13},
	{18, 8, 4, 3,14},
	{24,17,16,15,23}
};

PRIVATE uint8_t dith_c6w[6][6] = {
	{36,32,23,24,25,33},
	{31,15, 9,10,16,26},
	{22, 8, 2, 3,11,17},
	{21, 7, 1, 4,12,18},
	{30,14, 6, 5,13,27},
	{35,29,20,19,28,34}
};

PRIVATE uint8_t dith_c7w[7][7] = {
	{47,41,32,22,33,44,48},
	{40,26,17,11,18,27,43},
	{31,16, 6, 2, 7,19,34},
	{25,10, 5, 1, 3,12,23},
	{30,15, 9, 4, 8,20,35},
	{39,29,14,13,21,28,44},
	{46,38,37,24,36,45,49}
};

struct dith_descriptor {
	const char *name;
	const char *desc;		
	uint8_t *value;
	uint8_t mx;
	uint8_t my;
	uint8_t max;
};

#define DITH_DESCRIPTOR(name, max, mat, desc) \
	{name, desc, &mat[0][0], length_of(mat[0]), length_of(mat), \
	max ? max : length_of(mat[0])*length_of(mat)}

PRIVATE struct dith_descriptor dith_descriptors[] = {
	DITH_DESCRIPTOR("none",   1, dith_threshold, "Threshold"),
	
	DITH_DESCRIPTOR("checks", 2, dith_checks,    "Checkerboard 2x2 (dither)"),
	
	DITH_DESCRIPTOR("o2",     0, dith_o2,        "Ordered 2x2 (dispersed)"),
	DITH_DESCRIPTOR("o3",     0, dith_o3,        "Ordered 3x3 (dispersed)"),
	DITH_DESCRIPTOR("o4",     0, dith_o4,        "Ordered 4x4 (dispersed)"),
	DITH_DESCRIPTOR("o8",     0, dith_o8,        "Ordered 8x8 (dispersed)"),

	DITH_DESCRIPTOR("h4a",    8, dith_h4a,       "Halftone 4x4 (angled)"),
	DITH_DESCRIPTOR("h6a",   18, dith_h6a,       "Halftone 6x6 (angled)"),
	DITH_DESCRIPTOR("h8a",   32, dith_h8a,       "Halftone 8x8 (angled)"),

	DITH_DESCRIPTOR("h4o",    0, dith_h4o,       "Halftone 4x4 (orthogonal)"),
	DITH_DESCRIPTOR("h6o",    0, dith_h6o,       "Halftone 6x6 (orthogonal)"),
	DITH_DESCRIPTOR("h8o",    0, dith_h8o,       "Halftone 8x8 (orthogonal)"),

	DITH_DESCRIPTOR("c5b",    0, dith_c5b,       "Circles 5x5 (black)"),
	DITH_DESCRIPTOR("c6b",    0, dith_c6b,       "Circles 6x6 (black)"),
	DITH_DESCRIPTOR("c7b",    0, dith_c7b,       "Circles 7x7 (black)"),

	DITH_DESCRIPTOR("c5w",    0, dith_c5w,       "Circles 5x5 (white)"),
	DITH_DESCRIPTOR("c6w",    0, dith_c6w,       "Circles 6x6 (white)"),
	DITH_DESCRIPTOR("c7w",    0, dith_c7w,       "Circles 7x7 (white)"),

	DITH_DESCRIPTOR("vac",    0, dith_vac,       "Void and cluster (8x8)"),
	
	{NULL}
}, *dith_descriptor = &dith_descriptors[4];

PRIVATE uint8_t exo = 0;
PRIVATE uint8_t verbose = 0, pgm = 0, png = 0;
PRIVATE char *input_file, *output_file;

PRIVATE uint8_t centered = 1, hq_zoom = 1;
PRIVATE float aspect_ratio = 4.0f/3.0f;

typedef float vec3[3];

PRIVATE vec3 *vec3_set(vec3 *t, float x, float y, float z) {
	(*t)[0] = x;
	(*t)[1] = y;
	(*t)[2] = z;	
	return t;
}

PRIVATE vec3 *vec3_madd(vec3 *t, vec3 *u, float k, vec3 *v) {
	int i;
	for(i=0; i<3; ++i) (*t)[i] = (*u)[i] + k*(*v)[i];
	return t;
}

#define vec3_add(t, u, v) vec3_madd((t),(u),+1.0f,(v))
#define vec3_sub(t, u, v) vec3_madd((t),(u),-1.0f,(v))

PRIVATE vec3 *vec3_mul(vec3 *t, vec3 *u, vec3 *v) {
	return vec3_set(t,
		(*u)[1] * (*v)[2] - (*u)[2] * (*v)[1],
		(*u)[2] * (*v)[0] - (*u)[0] * (*v)[2],
		(*u)[0] * (*v)[1] - (*u)[1] * (*v)[0]);
}

PRIVATE float vec3_dot(vec3 *u, vec3 *v) {
	return (*u)[0] * (*v)[0] + (*u)[1] * (*v)[1] + (*u)[2] * (*v)[2];
}

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

PRIVATE color palette[15];

PRIVATE int float_cmp(float x, float y) {
	return x<y ? -1 : x>y ? +1 : 0;
}

PRIVATE int color_cmp_by_intens(const void *pa, const void *pb) {
	const color * const *a = pa, * const *b = pb;
	return float_cmp((*a)->intens, (*b)->intens);
}

PRIVATE int color_cmp_by_weight(const void *pa, const void *pb) {
	const color * const *a = pa, * const *b = pb;
	return -float_cmp((*a)->weight, (*b)->weight);
}

PRIVATE void set_palette(int i, float r, float g, float b) {
	color *c = &palette[i];
	vec3_set(&c->pt, r,g,b);
	c->index  = i;
	c->intens = 
		.3f*r + .59f*g + .11f*b;
		//.2126f*r +.7152f*g  + .0722f*b;
}

PRIVATE tetra tetras[27], *tetra_list;

PRIVATE void new_tetra(tetra *tetra, int a, int b, int c, int d) {
	vec3 p01,p02,p03,p12,p13;
	color **T = tetra->p;

	T[0] = &palette[a];
	T[1] = &palette[b];
	T[2] = &palette[c];
	T[3] = &palette[d];
			
	tetra->prev = NULL;
	tetra->next = tetra_list;
	if(tetra->next) 
	tetra->next->prev = tetra;
	tetra_list = tetra;
	
	vec3_sub(&p01, &T[1]->pt, &T[0]->pt);
	vec3_sub(&p02, &T[2]->pt, &T[0]->pt);
	vec3_sub(&p03, &T[3]->pt, &T[0]->pt);
	vec3_sub(&p12, &T[2]->pt, &T[1]->pt);
	vec3_sub(&p13, &T[3]->pt, &T[1]->pt);
	
	vec3_mul(&tetra->n012, &p01, &p02);
	vec3_mul(&tetra->n023, &p02, &p03);
	vec3_mul(&tetra->n031, &p03, &p01);
	vec3_mul(&tetra->n132, &p13, &p12);
	
	assert(vec3_dot(&tetra->n012, &p03)>=0);
}

/* https://www.geometrictools.com/Documentation/DistancePoint3Triangle3.pdf */
PRIVATE void tetra_proj3(float *w0, float *w1, float *w2, 
                         vec3 *point, vec3 *v0, vec3 *v1, vec3 *v2) {
	vec3 diff, edge0, edge1;
	float a00,a01,a11,b0,b1,det,t0,t1;
	
	vec3_sub(&diff, point, v0);
	vec3_sub(&edge0, v1, v0);
	vec3_sub(&edge1, v2, v0);
	
	a00 = vec3_dot(&edge0, &edge0);
	a01 = vec3_dot(&edge0, &edge1);
	a11 = vec3_dot(&edge1, &edge1);
	
	b0 = -vec3_dot(&diff, &edge0);
	b1 = -vec3_dot(&diff, &edge1);
	
	t0  = a01 * b1  - a11 * b0;
	t1  = a01 * b0  - a00 * b1;
	det = a00 * a11 - a01 * a01;
	
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

PRIVATE void tetra_proj2(float *w0, float *w1, vec3 *p, vec3 *v0, vec3 *v1) {
	vec3 v10, q;
	float t;
	
	vec3_sub(&v10, v1, v0);
	t = vec3_dot(&v10, &v10);
	
	if(t<=0) {
		t = 0;
	} else {
		t = vec3_dot(vec3_sub(&q, p, v0),&v10)/t;
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
	
	// if(w0<=0 && w1<=0 && w2<=0 && w3<=0) {
		// w0 = -w0; w1 = -w1; 
		// w2 = -w2; w3 = -w3;
	// }
	if(w0>=0 && w1>=0 && w2>=0 && w3>=0) {
		float  tot = w0 + w1 + w2 + w3;
		tot = tot<=0 ? 0 : 1.0f/tot;
		w0 *= tot; w1 *= tot; w2 *= tot; w3 *= tot;
	} else if(w0 < 0) {
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
	
	T[0]->weight = w0;
	T[1]->weight = w1;
	T[2]->weight = w2;
	T[3]->weight = w3;
	
	if(b != NULL) {
		int i;
		for(i=0;i<3;++i) {
			float x = w0*T[0]->pt[i] + w1*T[1]->pt[i]
		                + w2*T[2]->pt[i] + w3*T[3]->pt[i];
			
			(*b)[i] = x<0.0f ? 0.0f : x>1.0f ? 1.0f : x;
		}
	}
}

PRIVATE float sRGB2lin(uint8_t sRGB) {
	static float tab[256];
	
	if(tab[255]==0) {
		int i;
		for(i=0;i<256;++i) {
			float x = i/255.0f;
			tab[i] = x<=0.04045f ? x/12.92f : powf((x+0.055f)/1.055f, 2.4f);
		}
	}
	
	return tab[sRGB];
}	

PRIVATE uint32_t dith_key(vec3 *p) {
	const int base = 32;
	return  (uint32_t)(((*p)[0]<=0 ? 0 : (*p)[0])*(base-1)) +
		(uint32_t)(((*p)[1]<=0 ? 0 : (*p)[1])*(base-1))*base +
		(uint32_t)(((*p)[2]<=0 ? 0 : (*p)[2])*(base-1))*base*base;
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
		
		vec3_sub(&q, &q, p);
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
		if(best_t->next) 
		best_t->next->prev = best_t->prev;
		best_t->prev->next = best_t->next;
		
		tetra_list->prev = best_t;
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

PRIVATE uint8_t dith(const struct dith_descriptor *dith, 
                     const int x, const int y, vec3 *p) {
	const uint32_t key = dith_key(p);
	struct dith_cache *cache = hmgetp_null(dith_cache, key);
	
	assert(dith->max <= length_of(cache->value));

	if(cache == NULL) {
		tetra *t = dith_find_tetra(p);
		color *sel[4] = {t->p[0], t->p[1], t->p[2], t->p[3]},
		      *tab[length_of(cache->value)];
		      	
		qsort(sel, 4, sizeof(sel[0]), color_cmp_by_weight);
		// printf("%g %g %g %g\n", sel[0]->weight,sel[1]->weight,sel[2]->weight,sel[3]->weight);
		// printf("%d %d %d %d\n", sel[0]->index,sel[1]->index,sel[2]->index,sel[3]->index);
		// vec3 <q; 
		// tetra_coord(t, p, &q);
		// printf("%g %g %g\n", (*p)[0], (*p)[1], (*p)[2]);
		// printf("%g %g %g\n", q[0], q[1], q[2]);
		// vec3_sub(&q,&q,p);
		// printf("%g\n", vec3_dot(&q,&q));
		
		do {
			int i = 0; float m = 0.5;			
			m += sel[0]->weight * dith->max; while(i<m) tab[i++] = sel[0];
			m += sel[1]->weight * dith->max; while(i<m) tab[i++] = sel[1];
			m += sel[2]->weight * dith->max; while(i<m) tab[i++] = sel[2];
			while(i<dith->max) tab[i++] = sel[3];
		} while(0);
		
		qsort(tab, dith->max, sizeof(tab[0]), color_cmp_by_intens);
		
		do {
			struct dith_cache new_entry; int i;
			new_entry.key = key;
			for(i=0; i<dith->max; ++i) new_entry.value[i] = tab[i]->index;
			hmputs(dith_cache, new_entry);
		} while(0);
		
		cache = hmgetp_null(dith_cache, key);
		assert(cache != NULL);

		// for(int i=0;i<64;++i) printf("%d ", cache->value[i]);
		// printf("\n");
		// exit(0);
	}
	
	return cache->value[dith->value[(y % dith->my)*dith->mx + (x % dith->mx)]-1];
}

typedef struct {
	int w;
	int h;
	uint8_t *sRGB;
	uint8_t bitmap[65536];
	struct timeval time;
	int saved_size;
} pic;

PRIVATE float pic_done(pic *pic) {
	struct timeval now;
	float secs = 0;
	
	if(pic->sRGB) {
		free(pic->sRGB);
		pic->sRGB = NULL;
	
		gettimeofday(&now, NULL);
		if(now.tv_usec<pic->time.tv_usec) {
			now.tv_usec += 1000000;
			now.tv_sec--;
		}
		pic->time.tv_sec  = now.tv_sec  - pic->time.tv_sec;
		pic->time.tv_usec = now.tv_usec - pic->time.tv_usec;
		secs = pic->time.tv_sec + pic->time.tv_usec/1000000.0f;
	}
		
	if(verbose) {
		if(secs<0.0001) printf("done (%.1fus", secs*1000000.0f);
		else if(secs<1) printf("done (%.1fms", secs*1000.0f);
		else            printf("done (%.1fs",  secs);
		if(pic->saved_size>0) printf(", %d bytes", pic->saved_size);
		printf(")\n");
	}
	
	return secs;
}

PRIVATE const char *basename(const char *s) {
	const char *t = s;
	while(*t) ++t;
	while(t>s && t[-1]!='/' && t[-1]!='\\') --t;
	return t;
}

PRIVATE void squale_coord(pic *pic, int x, int y, int *rx, int *ry) {
	const int w = pic->w, h = pic->h;
	float fx, fy;
	if(w<=h*aspect_ratio) {
		fx = (x*h*aspect_ratio)/256  + (centered ? 0.5f*(w - h*aspect_ratio) : 0);
		fy = (y*h)/256;
	} else {
		fx = (x*w)/(256);
		fy = (y*w/aspect_ratio)/(256) + (centered ? 0.5f*(h - w/aspect_ratio) : 0);
	}
	*rx = nearbyintf(fx);
	*ry = nearbyintf(fy);
}

PRIVATE void pic_load(pic *pic, const char *filename) {
	int n;
	
	gettimeofday(&pic->time, NULL);
	pic->saved_size = 0;
	
	if(!stbi_info(filename, &pic->w, &pic->h, &n)) {
		fprintf(stderr, "Unsupported image: %s\n", filename);
		exit(-1);
	}
	
	if(verbose) {	
		printf("%s (%dx%d)...", basename(filename), pic->w, pic->h);
		fflush(stdout);
	}
	
	pic->sRGB = stbi_load(filename, &pic->w, &pic->h, &n, 3);
	
	if(pic->sRGB == NULL)  {
		if(verbose) printf("error\n");
		else fprintf(stderr, "Error while loading: %s\n", filename);
		exit(-1);
	}
	
#ifdef STBIR_INCLUDE_STB_IMAGE_RESIZE2_H
	if(hq_zoom) {
		int w, h;
		squale_coord(pic, 256, 256, &w, &h);
		
		if(aspect_ratio>=1) {
			h = (h*256)/w;
			w = 256;
		} else {
			w = (w*256)/h;
			h = 256;
		}
		
		if(w!=pic->w || h!=pic->h) {
			uint8_t *buf = stbir_resize_uint8_srgb(
				pic->sRGB, pic->w, pic->h,0,
				NULL,w,h,0, STBIR_RGB);
			if(buf) {
				free(pic->sRGB);
				pic->sRGB = buf;
				pic->w = w;
				pic->h = h;
			} 
		}
	}
#endif
}

PRIVATE void pic_save(pic *pic, const char *filename) {
	FILE *f = fopen(filename, "wb");
	
	if(f==NULL) {
		perror(filename);
		return;
	}
	
	if(verbose>1) {
		printf("saving %s...", basename(filename));
		fflush(stdout);
	}
	
	fprintf(f, exo ? "SQP\2" : "SQP\1");
	
	if(exo) {
		struct membuf inbuf[1];
		struct membuf outbuf[1];
		struct crunch_info info[1];
		static struct crunch_options options[1] = { CRUNCH_OPTIONS_DEFAULT };	
		int i = 65536;

		membuf_init(inbuf);
		do {
			--i;
			membuf_append(inbuf, &pic->bitmap[i^0xFF00], 1);
		} while(i);

		membuf_init(outbuf);
	        crunch(inbuf, outbuf, options, info);		
		fwrite(membuf_get(outbuf), 1, membuf_memlen(outbuf), f);
		membuf_free(outbuf);
		
		membuf_free(inbuf);
	} else {
		int i = 65536;
		do {
			i -= 2;
			fputc(pic->bitmap[i^0xFF00]*16+pic->bitmap[(i^0xFF00)+1], f);
		} while(i);
	}
	fflush(f);
	pic->saved_size = ftell(f);

	fclose(f);
}

PRIVATE void pic_save_pgm(pic *pic, const char *filename) {
	FILE *f = fopen(filename, "wt");
	int i;
	
	if(f==NULL) {
		perror(filename);
		return;
	}
	
	if(verbose>1) {
		printf("saving pgm...");
		fflush(stdout);
	}
	
	fprintf(f, "P3\n256 256\n255\n");
	for(i=0;i<65536;++i) {
		int c = pic->bitmap[i]>=8 ? HALF_INTENSITY : FULL_INTENSITY;
		fprintf(f, "%d %d %d\n", 
			pic->bitmap[i] & 4 ? 0 : c,
			pic->bitmap[i] & 2 ? 0 : c,
			pic->bitmap[i] & 1 ? 0 : c);
	}
	fclose(f);
}

PRIVATE void pic_save_png(pic *pic, const char *filename) {
	FILE *f = fopen(filename, "wt");
	uint8_t *buf = malloc(3*256*256), *rgb = buf;
	int i;

	if(buf==NULL) return;
	
	if(f==NULL) {
		perror(filename);
		return;
	}
	
	if(verbose>1) {
		printf("saving png...");
		fflush(stdout);
	}
	
	for(i=0;i<65536;++i) {
		int c = pic->bitmap[i]>=8 ? HALF_INTENSITY : FULL_INTENSITY;
		*rgb++ = pic->bitmap[i] & 4 ? 0 : c;
		*rgb++ = pic->bitmap[i] & 2 ? 0 : c;
		*rgb++ = pic->bitmap[i] & 1 ? 0 : c;
	}

	stbi_write_force_png_filter = 0;
	stbi_write_png_compression_level = 1024;
	stbi_write_png(filename, 256, 256, 3, buf, 3*256);
     
	free(buf);
	fclose(f);
}


PRIVATE vec3 *pic_get_linear_color(pic *pic, int x, int y, vec3 *ret) {
	
	if(x<0 || y<0 || x>=pic->w || y>=pic->h) {
		vec3_set(ret, 0,0,0);
	} else {
		uint8_t *img = pic->sRGB + 3*(pic->w*y + x);
		vec3_set(ret, sRGB2lin(img[0]), sRGB2lin(img[1]), sRGB2lin(img[2]));
	}
	
	// vec3_set(ret, sRGB[112], sRGB[23], sRGB[25]);
	// vec3_set(ret, 0.20792611981001, 0.02084547344562, 0.022794021361973);
	
	return ret;
}

PRIVATE vec3 *squale_color(pic *pic, int x, int y, vec3 *ret) {
	int x1, y1, x2, y2, i, j;
	float k; vec3 p;
	
	if((pic->w==256 && pic->w>=pic->h)
	|| (pic->h==256 && pic->h>=pic->w)) {
		squale_coord(pic, x,y, &x1, &y1);
		return pic_get_linear_color(pic, x1, y1, ret);
	}	
	
	squale_coord(pic, x-1,y-1, &x1, &y1);
	squale_coord(pic, x+1,y+1, &x2, &y2);
	
	k = 1.0f/((1.0f+y2-y1)*(1.0f+x2-x1));
	
	vec3_set(ret, 0,0,0);
	for(i=x1; i<=x2; ++i) for(j = y1; j<=y2; ++j) {
		vec3_madd(ret, ret, k, pic_get_linear_color(pic, i,j, &p));
	}
	
	// (*ret)[0] = x/255.0f;	
	// (*ret)[1] = y/255.0f;	
			
	return ret;
}

PRIVATE void pic_dither(pic *pic, int x, int y) {
	vec3 p; 
	
	uint8_t c = dith(dith_descriptor,  x & 255, y & 255, 
	  		 squale_color(pic, x & 255, y & 255, &p));
	
	pic->bitmap[x + y*256] = c; //*0+(((x/30)+(y/30))%14);
}

// hilbert cuve improve cache hits
PRIVATE void pic_conv_h(pic *pic) {
	static int dir[] = {256,1,-256,-1}, a, p = 0, l = 8, b = 1;
	if(pic==NULL)  {
		l = 8;
		b = 1;
		p = 0;
	} else if(l == 0) {
		pic_dither(pic, p & 255, p>>8);
	} else {
		--l;
		a -= (b=-b); 
		pic_conv_h(pic);
		p = p + dir[a&3]; 
		a -= (b=-b); 
		pic_conv_h(pic);
		p = p + dir[a&3]; 
		pic_conv_h(pic);
		a += (b=-b); 
		p = p + dir[a&3]; 
		pic_conv_h(pic); 
		a += (b=-b);		
		++l;
	}
}

PRIVATE void pic_conv_l(pic *pic) {
	int i;
	for(i=0;i<65536;++i) pic_dither(pic, i & 255, i>>8);
}

PRIVATE void init(void) {
	static uint16_t tetras_desc[] = {
#if HALF_INTENSITY==187
		0x0518, 0x0458, 0x0248, 0x4268, 
		0x0328, 0x0138, 0x1389, 0x5189, 
		0x682A, 0x328A, 0x3A8B, 0x8A7B, 
		0x879B, 0x389B, 0x584C, 0x486C, 
		0x87CD, 0x58CD, 0x897D, 0x598D, 
		0x78CE, 0x6C8E, 0x68AE, 0x7A8E
#elif HALF_INTENSITY==157
		0x4268, 0x0328, 0x0138, 0x5048, 
		0x0248, 0x5108, 0x1389, 0x5189, 
		0x283A, 0x268A, 0x879B, 0x389B, 
		0x8A7B, 0x3A8B, 0x486C, 0x584C, 
		0x58CD, 0x598D, 0x789D, 0x7C8D, 
		0x86CE, 0x7A8E, 0x8A6E, 0x78CE
#else
		// 0x0138, 0x0248, 0x4268, 0x0518, 
		// 0x0458, 0x0328, 0x1389, 0x1859, 
		// 0x283A, 0x268A, 0x78AB, 0x798B, 
		// 0x389B, 0x3A8B, 0x486C, 0x458C, 
		// 0x859D, 0x789D, 0x7C8D, 0x8C5D, 
		// 0x8A6E, 0x7A8E, 0x78CE, 0x86CE
		// 0x2648, 0x0328, 0x1058, 0x0248, 0x0458, 0x1308, 0x1389, 0x1859, 0x328A, 0x268A, 0x78AB, 0x798B, 0x389B, 0x3A8B, 0x486C, 0x458C, 0x789D, 0x859D, 0x7C8D, 0x8C5D, 0x68AE, 0x7A8E, 0x78CE, 0x6C8E
		// 0x1048, 0x2138, 0x2608, 0x0648, 0x1458, 0x2018, 0x1859, 0x1389, 0x268A, 0x283A, 0x3A8B, 0x78AB, 0x389B, 0x798B, 0x584C, 0x486C, 0x8C5D, 0x859D, 0x7C8D, 0x789D, 0x86CE, 0x8A6E, 0x78CE, 0x7A8E
		// 0x0268, 0x5108, 0x0648, 0x1208, 0x1328, 0x5048, 0x1389, 0x1859, 0x283A, 0x268A, 0x879B, 0x389B, 0x8A7B, 0x3A8B, 0x486C, 0x458C, 0x897D, 0x598D, 0x87CD, 0x58CD, 0x7A8E, 0x6C8E, 0x78CE, 0x68AE
		// 0x0138, 0x0328, 0x4518, 0x6408, 0x0418, 0x6028, 0x5189, 0x1389, 0x832A, 0x826A, 0x389B, 0x78AB, 0x3A8B, 0x798B, 0x648C, 0x584C, 0x58CD, 0x789D, 0x598D, 0x7C8D, 0x8C7E, 0x6C8E, 0x68AE, 0x87AE
		// 0x6428, 0x0458, 0x3208, 0x3018, 0x0518, 0x0248, 0x1859, 0x3819, 0x283A, 0x682A, 0x3A8B, 0x78AB, 0x389B, 0x798B, 0x458C, 0x486C, 0x598D, 0x58CD, 0x7C8D, 0x789D, 0x68AE, 0x6C8E, 0x78CE, 0x7A8E
		0x0128, 0x0518, 0x0268, 0x0648, 0x1328, 0x0458, 0x1389, 0x1859, 0x328A, 0x826A, 0x3A8B, 0x78AB, 0x389B, 0x798B, 0x458C, 0x648C, 0x789D, 0x58CD, 0x598D, 0x7C8D, 0x86CE, 0x78CE, 0x7A8E, 0x8A6E
#endif
	};
	int i;
	
	stbds_rand_seed(time(0));	
	
	for(i=0; i<15; ++i) {
		float c = sRGB2lin(i>=8 ? HALF_INTENSITY : FULL_INTENSITY);
		set_palette(i, i&4 ? 0.0f : c,
		               i&2 ? 0.0f : c,
			       i&1 ? 0.0f : c);
	}

	for(i=0; i<length_of(tetras_desc); ++i) {
		uint16_t x = tetras_desc[i];
		new_tetra(&tetras[i], 
			  x>>12,(x>>8)&15,
			  (x>>4)&15,x&15);
	}
}

PRIVATE void usage(char *av0) {
	int i;
	
	printf("Usage: %s [options] <image.ext>\n", av0);
	printf("options:\n");
	printf(" ?, -h, --help : Prints this help\n");

	printf(" -v            : Verbose\n");
	printf(" -o <name>     : Specify output file\n");
	printf(" -x            : same as --o4\n");
	printf(" -z            : same as --exo\n");
	printf("\n");
	
	printf(" --exo         : Compresses with exomizer\n");
	printf(" --png         : Output png image (for preview)\n");
	printf(" --pgm         : Output pgm image (for preview)\n");
	printf(" --ratio <num> : Sets aspect ratio\n");
	printf("\n");
	
	for(i=0; dith_descriptors[i].name; ++i)
	printf(" --%-11s : %s\n", dith_descriptors[i].name, dith_descriptors[i].desc);
	
	exit(0);
}

PRIVATE void parse(int ac, char **av) {
	int i;
	for(i=1; i<ac; ++i) {
		if(!strcmp("?", av[i])
		|| !strcmp("-h", av[i])
		|| !strcmp("--help", av[i])
		|| 0) usage(av[0]);

		else if(!strcmp("-v", av[i])) 
			verbose = 1;
		else if(!strcmp("-o", av[i]) && i<ac-1)
			output_file = av[++i];
		else if(!strcmp("-x", av[i])) 
			dith_descriptor = &dith_descriptors[4];
		else if(!strcmp("--exo", av[i])
                     || !strcmp("-z",   av[i]))
			exo = 1;
		else if(!strcmp("--pgm", av[i])) 
			pgm = 1;
		else if(!strcmp("--png", av[i])) 
			png = 1;
		else if(!strcmp("--ratio", av[i]) && i<ac-1)
			aspect_ratio = atof(av[++i]);
		else if(!strncmp("--", av[i], 2)) {
			int j;
			for(j=0; dith_descriptors[j].name;++j) {
				if(!strcmp(dith_descriptors[j].name, av[i]+2)) {
					dith_descriptor = &dith_descriptors[j];
					break;
				}
			}
		}
		else if(input_file == NULL)
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
}

int main(int ac, char **av) {
	parse(ac, av);
	
	init();
	do {
		pic pic;
		char *out;
		
		pic_load(&pic, input_file);
		pic_conv_h(NULL);
		pic_conv_h(&pic);
		out = output_file;

		if(out==NULL) {
			char *s;
			out = malloc(strlen(input_file)+5);
			if(out==NULL) {
				perror("Out of memory");
				exit(-1);
			}
			strcpy(out, input_file);
			s = strrchr(out, '.');
			if(s) *s = 0;
			strcat(s, ".SQP");
		}

		pic_save(&pic, out);
		if(pgm) {
			char *s = malloc(strlen(out)+5);
			if(s) {
				strcpy(s, out);
				strcat(s, ".pgm");
				pic_save_pgm(&pic, s);
				free(s);
			}
		}
		if(png) {
			char *s = malloc(strlen(out)+5);
			if(s) {
				strcpy(s, out);
				strcat(s, ".png");
				pic_save_png(&pic, s);
				free(s);
			}
		}
		pic_done(&pic);
		if(out != output_file) free(out);
	} while(0);
	
	return 0;
}