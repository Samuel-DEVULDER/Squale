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

#include "gifenc/gifenc.h"

#include "membuf_io.h"
#include "exo_helper.h"

#define length_of(array) (sizeof(array)/sizeof(array[0]))

#ifndef TRUE
#define TRUE    1
#define FALSE   0
#endif

#define PRIVATE static

#define FATAL(fmt, value, num) do {		\
	fprintf(stderr, fmt"\n", value); 	\
	if(num) exit(num); 			\
	} while(0)

#define OUT_OF_MEM(n) 	FATAL("Out of memory: %d bytes", n, -1);

#define FULL_INTENSITY	255
//#define HALF_INTENSITY	187
#define HALF_INTENSITY	157
// #define HALF_INTENSITY	127

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

// https://perso.liris.cnrs.fr/victor.ostromoukhov/publications/pdf/SPIE95_RotatedNonBayer.pdf

// PRIVATE uint8_t dith_hexx[18][12] = {
	// {  1, 55,100, 46,  6, 60,105, 51,  9, 63,108, 54},
	// { 91, 37, 19, 73, 96, 42, 24, 78, 99, 45, 27, 81},
	// { 10, 64, 88, 34, 15, 69, 83, 29, 18, 72, 85, 31}, 
	// {103, 49,  7, 61,106, 54,  2, 56,101, 47,  4, 58}, 
	// { 22, 76, 97, 43, 25, 79, 92, 38, 20, 74, 94, 40}, 
	// { 84, 30, 16, 70, 86, 32, 11, 65, 89, 35, 13, 67},
	// {  3, 57,102, 48,  5, 59,104, 50,  8, 62,107, 53},
	// { 93, 39, 21, 75, 95, 41, 23, 77, 98, 44, 26, 80},
	// { 12, 66, 90, 36, 17, 68, 82, 28, 17, 71, 87, 33},
	// {105, 51,  9, 63,108, 54,  1, 55,100, 46,  6, 60}, 
	// { 24, 78, 99, 45, 27, 81, 91, 37, 19, 73, 96, 42},
	// { 83, 29, 18, 72, 85, 31, 10, 64, 88, 34, 15, 69},
	// {  2, 56,101, 47,  4, 58,103, 49,  7, 61,106, 52},
	// { 92, 38, 20, 74, 94, 40, 22, 76, 97, 43, 25, 79},
	// { 11, 65, 89, 35, 13, 67, 84, 30, 16, 70, 86, 32},
	// {104, 50,  8, 62,107, 53,  3, 57,102, 48,  5, 59},
	// { 23, 77, 98, 44, 26, 80, 93, 39, 21, 75, 95, 41},
	// { 82, 28, 17, 71, 87, 33, 12, 66, 90, 36, 14, 68}
// }; 

PRIVATE uint8_t dith_hex[12][18] = {
	{  1, 91, 10,103, 22, 84,  3, 93, 12,105, 24, 83,  2, 92, 11,104, 23, 82},
	{ 55, 37, 64, 49, 76, 30, 57, 39, 66, 51, 78, 29, 56, 38, 65, 50, 77, 28},
	{100, 19, 88,  7, 97, 16,102, 21, 90,  9, 99, 18, 101, 20, 89, 8, 98, 17},
	{ 46, 73, 34, 61, 43, 70, 48, 75, 36, 63, 45, 72, 47, 74, 35, 62, 44, 71},
	{  6, 96, 15,106, 25, 86,  5, 95, 17,108, 27, 85,  4, 94, 13,107, 26, 87},
	{ 60, 42, 69, 54, 79, 32, 59, 41, 68, 54, 81, 31, 58, 40, 67, 53, 80, 33},
	{105, 24, 83,  2, 92, 11,104, 23, 82,  1, 91, 10,103, 22, 84,  3, 93, 12},
	{ 51, 78, 29, 56, 38, 65, 50, 77, 28, 55, 37, 64, 49, 76, 30, 57, 39, 66},
	{  9, 99, 18, 101, 20, 89, 8, 98, 17,100, 19, 88,  7, 97, 16,102, 21, 90},
	{ 63, 45, 72, 47, 74, 35, 62, 44, 71, 46, 73, 34, 61, 43, 70, 48, 75, 36},
	{108, 27, 85,  4, 94, 13,107, 26, 87,  6, 96, 15,106, 25, 86,  5, 95, 14},
	{ 54, 81, 31, 58, 40, 67, 53, 80, 33, 60, 42, 69, 52, 79, 32, 59, 41, 68}
};

// 36
PRIVATE uint8_t dith_h3r[30][30] = {
	{36, 8,24,30,26, 7,17,29, 1, 9,18,34, 2,22,16,33, 5,21,31,27, 6,20,32, 4,12,19,35, 3,23,13},
	{32,16,28, 5,31, 3,27,11,20, 4,24,12,14,35,23,29,13,25, 8,30, 2,26,10,17, 1,21, 9,15,34,22},
	{10,17,33, 1,21,15,34, 6,22,32,28, 5,19,31, 3,11,20,36, 4,24,14,35, 7,23,29,25, 8,18,30, 2},
	{35, 7,29,25, 9, 8,18, 2,10,16,17,33,21,15,27,34, 6,32,28,12, 5,19, 3,11,13,20,36,24,14,26},
	{ 3,23,11,13,36,24,30,14,26, 7,29, 1,25, 9,18, 2,22,10,16,33,21,31,15,27, 6,32, 4,28,12,19},
	{31,27, 6,20,32, 4,12,19,35, 3,23,13,36, 8,24,30,26, 7,17,29, 1, 9,18,34, 2,22,16,33, 5,21},
	{15,18,34,22,16,28,33, 5,31,27,11, 6,20, 4,12,14,19,35,23,13,25,36, 8,30,26,10, 7,17, 1, 9},
	{ 8,30, 2,26,10,17, 1,21, 9,15,34,22,32,16,28, 5,31, 3,27,11,20, 4,24,12,14,35,23,29,13,25},
	{ 4,24,14,35, 7,23,29,25, 8,18,30, 2,10,17,33, 1,21,15,34, 6,22,32,28, 5,19,31, 3,11,20,36},
	{28,12, 5,19, 3,11,13,20,36,24,14,26,35, 7,29,25, 9, 8,18, 2,10,16,17,33,21,15,27,34, 6,32},
	{16,33,21,31,15,27, 6,32, 4,28,12,19, 3,23,11,13,36,24,30,14,26, 7,29, 1,25, 9,18, 2,22,10},
	{17,29, 1, 9,18,34, 2,22,16,33, 5,21,31,27, 6,20,32, 4,12,19,35, 3,23,13,36, 8,24,30,26, 7},
	{23,13,25,36, 8,30,26,10, 7,17, 1, 9,15,18,34,22,16,28,33, 5,31,27,11, 6,20, 4,12,14,19,35},
	{27,11,20, 4,24,12,14,35,23,29,13,25, 8,30, 2,26,10,17, 1,21, 9,15,34,22,32,16,28, 5,31, 3},
	{34, 6,22,32,28, 5,19,31, 3,11,20,36, 4,24,14,35, 7,23,29,25, 8,18,30, 2,10,17,33, 1,21,15},
	{18, 2,10,16,17,33,21,15,27,34, 6,32,28,12, 5,19, 3,11,13,20,36,24,14,26,35, 7,29,25, 9, 8},
	{30,14,26, 7,29, 1,25, 9,18, 2,22,10,16,33,21,31,15,27, 6,32, 4,28,12,19, 3,23,11,13,36,24},
	{12,19,35, 3,23,13,36, 8,24,30,26, 7,17,29, 1, 9,18,34, 2,22,16,33, 5,21,31,27, 6,20,32, 4},
	{33, 5,31,27,11, 6,20, 4,12,14,19,35,23,13,25,36, 8,30,26,10, 7,17, 1, 9,15,18,34,22,16,28},
	{ 1,21, 9,15,34,22,32,16,28, 5,31, 3,27,11,20, 4,24,12,14,35,23,29,13,25, 8,30, 2,26,10,17},
	{29,25, 8,18,30, 2,10,17,33, 1,21,15,34, 6,22,32,28, 5,19,31, 3,11,20,36, 4,24,14,35, 7,23},
	{13,20,36,24,14,26,35, 7,29,25, 9, 8,18, 2,10,16,17,33,21,15,27,34, 6,32,28,12, 5,19, 3,11},
	{ 6,32, 4,28,12,19, 3,23,11,13,36,24,30,14,26, 7,29, 1,25, 9,18, 2,22,10,16,33,21,31,15,27},
	{ 2,22,16,33, 5,21,31,27, 6,20,32, 4,12,19,35, 3,23,13,36, 8,24,30,26, 7,17,29, 1, 9,18,34},
	{26,10, 7,17, 1, 9,15,18,34,22,16,28,33, 5,31,27,11, 6,20, 4,12,14,19,35,23,13,25,36, 8,30},
	{14,35,23,29,13,25, 8,30, 2,26,10,17, 1,21, 9,15,34,22,32,16,28, 5,31, 3,27,11,20, 4,24,12},
	{19,31, 3,11,20,36, 4,24,14,35, 7,23,29,25, 8,18,30, 2,10,17,33, 1,21,15,34, 6,22,32,28, 5},
	{21,15,27,34, 6,32,28,12, 5,19, 3,11,13,20,36,24,14,26,35, 7,29,25, 9, 8,18, 2,10,16,17,33},
	{25, 9,18, 2,22,10,16,33,21,31,15,27, 6,32, 4,28,12,19, 3,23,11,13,36,24,30,14,26, 7,29, 1},
	{20, 4,12,14,19,35,23,13,25,36, 8,30,26,10, 7,17, 1, 9,15,18,34,22,16,28,33, 5,31,27,11, 6}
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

	DITH_DESCRIPTOR("hex",  108, dith_hex,	     "Hexagonal (18x12)"),
	DITH_DESCRIPTOR("h3r",   36, dith_h3r,	     "Halftone 6x6 (rotated)"),
	
	{NULL}
}, *dith_descriptor;

PRIVATE uint8_t exo  = FALSE;
PRIVATE uint8_t verbose  = FALSE, pgm  = FALSE, png  = FALSE, gif = FALSE;
PRIVATE char *input_file, *output_file = "%p/%N.SQP";

PRIVATE uint8_t centered = 1, hq_zoom = 1,  hilbert = 1;
PRIVATE float aspect_ratio = 1.0f;

typedef float vec3[3];

PRIVATE char flex9char(char c) {
	if( c=='_' || c=='-'
	|| (c>='0' && c<='9')
	|| (c>='A' && c<='Z')) return c;
	if((c>='a' && c<='z')) return c+'A'-'a';
	return 0;
}

PRIVATE const char *path_format(const char *fmt, const char *path) {
	const char *ext = NULL, *name = NULL, *end = NULL;
	char *out = NULL, c;
	const char *s, *t;
	
	// fint name, ext, etc
	for(s = path; *s; ++s); 
	end = s;
	while(s>path) {
		--s;
		if(*s=='.' && ext==NULL) ext = s;
		else if(*s=='/' || *s=='\\') {
			name = s + 1;
			if(ext==NULL) ext = end;
			break;
		}
	}
	if(name==NULL) name = ext = end;
	
	// now format
	for(s = fmt; *s; ++s) {
		int n;
		
		if(*s=='%') {
			switch(*++s) {
			case '%': arrput(out , '%'); break;
				
			case 's': for(t = path; t<end; ++t) arrput(out, *t); break;
			case 'e': for(t = ext;  t<end; ++t) arrput(out, *t); break;
			case 'n': for(t = name; t<ext; ++t) arrput(out, *t); break;

			case 'P': case 'p': 
				if(path==name) {arrput(out,'.');arrput(out,'/');} 
				else for(t = path; t<name; ++t) {arrput(out, *t);} 
				break;
				
			case 'E':
				for(n = 0, t = ext; t<end && n<3; ++t) {
					char c = flex9char(*t);
					if(c) {arrput(out, c); ++n;}
				}
				break;
				
			case 'N':
				for(n = 0, t = name ;t<ext && n<8; ++t) {
					c = flex9char(*t);
					if(c) {
						if(n==0 && (c<'A' || c>'Z')) {
							arrput(out, 'Z');
							++n;
						}
						arrput(out, c); 
						++n;
					}
				}
				break;
				
			default:
				FATAL("Invalid format specifier : %%%c", *s, -1);
			}
		} else if(*s=='/' || *s=='\\') {
			n = arrlenu(out) - 1;
			if(n>0 && out[n]!='/' && out[n]!='\\') arrput(out, *s);
		} else arrput(out, *s);
	}
	arrput(out, '\0');
		
	s = strdup(out); arrfree(out);
	if(s==NULL) OUT_OF_MEM(arrlenu(out));
	return s;
}

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
	float *v = &(*p)[0];
	const int base = 128;
	return  ((uint32_t)((0.5f + v[0])*(base-1))) +
		((uint32_t)((0.5f + v[1])*(base-1)))*base +
		((uint32_t)((0.5f + v[2])*(base-1)))*base*base;
 }

PRIVATE struct dith_cache {
	uint32_t key;
	uint8_t  value[128];
} *dith_cache;
PRIVATE double dith_total, dith_hit;

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
	const uint32_t key = dith_key(p); if(key==0) return 7;
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
			int i = 0; float m = 0; //0.5f;			
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
	else dith_hit += 1; 
	dith_total += 1;

	return cache->value[dith->value[(y % dith->my)*dith->mx + (x % dith->mx)]-1];
}

PRIVATE struct dith_descriptor *dith_find(char *name) {
	int i;
	for(i=0; dith_descriptors[i].name;++i) {
		if(!strcmp(dith_descriptors[i].name, name)) {
			return &dith_descriptors[i];
		}
	}
	return NULL;
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
	float fx, fy, k = aspect_ratio;
	if(w<=h*k) {
		fx = (x*h*k)/256  + (centered ? 0.5f*(w - h*k) : 0);
		fy = (y*h)/256;
	} else {
		fx = (x*w)/256;
		fy = (y*w/k)/256 + (centered ? 0.5f*(h - w/k) : 0);
	}
	*rx = nearbyintf(fx);
	*ry = nearbyintf(fy);
}

PRIVATE int pic_load(pic *pic, const char *filename) {
	int n;
	
	gettimeofday(&pic->time, NULL);
	pic->saved_size = 0;
	
	if(!stbi_info(filename, &pic->w, &pic->h, &n)) {
		FATAL("Unsupported image: %s", filename, 0);
		return FALSE;
	}
	
	if(verbose) {	
		printf("%s (%dx%d)...", basename(filename), pic->w, pic->h);
		fflush(stdout);
	}
	
	pic->sRGB = stbi_load(filename, &pic->w, &pic->h, &n, 3);
	
	if(pic->sRGB == NULL)  {
		FATAL("Error while loading: %s", filename, 0);
		return FALSE;
	}
	
#ifdef STBIR_INCLUDE_STB_IMAGE_RESIZE2_H
	if(hq_zoom) {
		int w = pic->w, h = pic->h;
		
		if(h>w*aspect_ratio) {
			w = (w*256)/h;
			h = 256;
		} else {
			h = (h*256)/w;
			w = 256;
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
	return TRUE;
}

PRIVATE uint32_t pic_crc32(pic *pic) {
	#define CRC32_POLY 0x04C11DB7
	uint32_t crc = ~0; int i, j;
	for(i = pic->w*pic->h*3; --i>=0;) {
		crc ^= ((uint32_t)pic->sRGB[i]) << 24; 
		for (j = 0; j < 8; ++j) {
			int32_t msb = crc & 0x80000000;	
			crc <<= 1;
			if(msb) crc ^= CRC32_POLY;
		}
        }
	return ~crc;
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

	if(buf==NULL) OUT_OF_MEM(3*256*256);
	
	if(f==NULL) {perror(filename); return;}
	
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

PRIVATE void pic_save_gif(pic *pic, const char *filename) {
	uint8_t palette[16*3];
	ge_GIF *gif;
	int i;
	
	for(i=0; i<16; ++i) {
		uint8_t c = (i>=8 ? HALF_INTENSITY : FULL_INTENSITY);
		palette[3*i + 0] = i&4 ? 0 : c;
		palette[3*i + 1] = i&2 ? 0 : c;
		palette[3*i + 2] = i&1 ? 0 : c;
	}

	gif = ge_new_gif(filename, 256, 256, palette, 4, -1, -1);
	if(!gif) {perror(filename); return;}

	if(verbose>1) {
		printf("saving gif...");
		fflush(stdout);
	}
	memcpy(gif->frame, pic->bitmap, sizeof(pic->bitmap));
	ge_add_frame(gif, 0);
	ge_close_gif(gif);
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
	
	++x1; if(x1>=x2) x2 = x1+1;
	++y1; if(y1>=y2) y2 = y1+1;
	
	k = 1.0f/((y2-y1)*(float)(x2-x1));
	

	vec3_set(ret, 0,0,0);
	for(i=x1; i<x2; ++i) for(j = y1; j<y2; ++j) {
		vec3_madd(ret, ret, k, pic_get_linear_color(pic, i,j, &p));
	}
	
	// (*ret)[0] = x/255.0f;	
	// (*ret)[1] = y/255.0f;	
	// (*ret)[2] = 0;	
			
	return ret;
}

PRIVATE void pic_dither(pic *pic, int x, int y) {
	vec3 p; 
	
	uint8_t c = dith(dith_descriptor,  x, y, 
	  		 squale_color(pic, x, y, &p));
	
	pic->bitmap[x + y*256] = c; //*0+(((x/30)+(y/30))%14);
}

// hilbert cuve improve cache hits
PRIVATE void pic_conv_h(pic *pic) {
	static int dir[] = {256,1,-256,-1}, a, l = 8, b = 1;
	static unsigned int p = 0;
	if(pic==NULL)  {
		l = 8;
		b = 1;
		p = 0;
		a = 0;
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
		0x0128, 0x0518, 0x0268, 0x0648, 
		0x1328, 0x0458, 0x1389, 0x1859, 
		0x328A, 0x826A, 0x3A8B, 0x78AB, 
		0x389B, 0x798B, 0x458C, 0x648C, 
		0x789D, 0x58CD, 0x598D, 0x7C8D, 
		0x86CE, 0x78CE, 0x7A8E, 0x8A6E
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
	
	dith_descriptor = dith_find("hex"); // this one seem pretty nice
}

PRIVATE void usage(char *av0) {
	int i;
	
	printf("Usage: %s [options] <image.ext> ...\n", av0);
	printf("options:\n");
	printf(" ?, -h, --help : Prints this help\n");

	printf(" -v            : Verbose\n");
	printf(" -o <name>     : Specify output file "
		"(accepted patterns: %%s, %%p, %%n, %%e, %%N, %%E)\n");
	printf(" -x            : same as --o4\n");
	printf(" -z            : same as --exo\n");
	printf(" -r <w:h>      : same as --ratio\n");
	printf("\n");
	
	printf(" --exo         : Compresses with exomizer\n");
	printf(" --gif         : Output gif image (for preview)\n");
	printf(" --png         : Output png image (for preview)\n");
	printf(" --pgm         : Output pgm image (for preview)\n");
	printf(" --low         : Low quality resizing\n");
	printf(" --ratio <w:h> : Sets aspect ratio (default=1:1)\n");
	printf("\n");
	
	for(i=0; dith_descriptors[i].name; ++i)
	printf(" --%-11s : %s\n", dith_descriptors[i].name, dith_descriptors[i].desc);
	
	exit(0);
}

PRIVATE int parse(int i, int ac, char **av) {
	for(input_file = NULL; i<ac && input_file==NULL; ++i) {
		if(!strcmp("?", av[i])
		|| !strcmp("-h", av[i])
		|| !strcmp("--help", av[i])
		|| 0) usage(av[0]);

		else if(!strcmp("-v", av[i])) 
			verbose = 1;
		else if(!strcmp("--debug", av[i])) 
			verbose = 2;
		else if(!strcmp("-o", av[i]) && i<ac-1)
			output_file = av[++i];
		else if(!strcmp("-x", av[i])) 
			dith_descriptor = dith_find("o4");
		else if(!strcmp("--exo", av[i])
                     || !strcmp("-z",   av[i]))
			exo = TRUE;
		else if(!strcmp("--pgm", av[i])) 
			pgm = TRUE;
		else if(!strcmp("--png", av[i])) 
			png = TRUE;
		else if(!strcmp("--gif", av[i])) 
			gif = TRUE;
		else if(!strcmp("--low", av[i])) 
			hq_zoom = FALSE;
		else if(i<ac-1 && (
			 !strcmp("--ratio", av[i]) ||
			 !strcmp("-r", av[i])
			 )) {
			char *s = av[++i];		
			float x,y;
			if(sscanf(s, "%f:%f", &x, &y) != 2) {
				y = 1.0f;
				if(sscanf(s,"%f", &x) != 1) {
					x = y = -1.0f;
				}
			}
			if(y<=0) {
				fprintf(stderr, "Invalid ratio: %s\n" , av[i]);
				exit(-1);
			}
			aspect_ratio = fabsf(x/y);
		} 
		else if(!strncmp("--", av[i], 2)) {
			dith_descriptor = dith_find(av[i]+2);
			if(!dith_descriptor) FATAL("Unknown dither: --%s", av[i]+2, -1);
		}
		else if(*av[i] != '-') {
			FILE *f = fopen(av[i], "rb");
			if(f) {fclose(f); input_file = av[i];}
			else perror(av[i]);
		}
		else FATAL("Unknown argument: %s" , av[i], -1);
	}
	if(input_file == NULL) FATAL("Missing file after : %s", av[ac-1], -1);
	return i;
}

int main(int ac, char **av) {
	int i = 1;
	
	if(ac==1) usage(av[0]);
	
	init();
	do {
		const char *out;
		pic pic;
		
		i = parse(i, ac, av);
		
		if(!pic_load(&pic, input_file)) continue;
		
		// convert
		if(hilbert) {
			pic_conv_h(NULL);
			pic_conv_h(&pic);
		} else pic_conv_l(&pic);
		if(hmlen(dith_cache)>=32768) {
			hmfree(dith_cache);
			dith_hit = dith_total = 0;
		}

		//save
		out = path_format(output_file, input_file);
		if(strstr(output_file, "%N") != NULL) {
			FILE *f = fopen(out, "rb");
			if(f) { fclose(f);
				int32_t crc = pic_crc32(&pic) % 1291;
				int n = strlen(out), l = n+3;
				char *tmp = malloc(l); if(!tmp) OUT_OF_MEM(l);
				strcpy(tmp, out);
								
				while(n>0 && tmp[n-1]!='/' && tmp[n-1]!='\\') --n;
				for(l=0; tmp[n+l] && tmp[n+l]!='.'; ++l);
				
				if(l<8) {
					int l2 = l;
					
					tmp[n + l++] = ' '; 
					if(l<8) tmp[n + l++] = ' '; 
					
					strcpy(tmp + n + l, out + n + l2);
				}
				n += l-2;
				l = crc%36; crc/=36; tmp[n++] = l<10? '0'+l : 'A'+l-10;
				l = crc%36; crc/=36; tmp[n++] = l<10? '0'+l : 'A'+l-10;
				free((void*)out);
				out = tmp;
			}
		}
		pic_save(&pic, out);
		
		// overview
		if(pgm) {
			const char *s = path_format("%s.pgm", out);
			pic_save_pgm(&pic, s);
			free((void*)s);
		}
		if(png) {
			const char *s = path_format("%s.png", out);
			pic_save_png(&pic, s);
			free((void*)s);
		}
		if(gif) {
			const char *s = path_format("%s.gif", out);
			pic_save_gif(&pic, s);
			free((void*)s);
		}
		
		// done
		if(verbose > 1) printf("hm=%d (%dkb, %.1f%%)...", 
			hmlen(dith_cache),hmlen(dith_cache)*sizeof(*dith_cache)/1024, 
			100*dith_hit/dith_total);
		pic_done(&pic);
		free((void*)out);
	} while(i<ac);
	
	return 0;
}