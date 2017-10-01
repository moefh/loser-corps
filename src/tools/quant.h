/* quant.h
 * Copyright (C) 1999 Denis M .Souza
 */

#ifndef QUANT_H_FILE
#define QUANT_H_FILE

#define MAX_COLORS  0x10000    /* Numero max de cores diferentes em uma img */
#define GREEN       0x07E0     /* Define o verde puro para 16bpp */
#define TRANSPARENT GREEN

typedef struct ColorSpace ColorSpace;

/*
 * This structure defines a "color space", that is, the set of all
 * colors being quantized at a given moment.  The color space is made
 * of all colors in the color vector v[i] where (i >= Start && i < End).
 *
 * The color spaces are put on a queue as they're created (the
 * implementation of the color spaces in a linked list eases their
 * manipulation).
 */
struct ColorSpace
{
  unsigned int Start;
  unsigned int End;
  ColorSpace   *next;
};


unsigned int ReadColors(unsigned int NFiles, char *Files[],
			unsigned int *Colors, unsigned int *Freq);
ColorSpace *QuantColors(unsigned int NColors, unsigned int *Colors,
			unsigned int *Freq, unsigned int NFColors);
void SaveColors(unsigned int NFiles, char *Files[], ColorSpace *CSpace,
		unsigned int NColors, unsigned int *Colors,
		unsigned int *Freq);
int Sort(ColorSpace *CSpace, unsigned char *V1, unsigned char *V2,
	 unsigned char *V3, unsigned int *V4, unsigned int *V5);

#endif /* QUANT_H_FILE */
