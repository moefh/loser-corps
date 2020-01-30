/* quant.c
 * Copyright (C) 1999 Denis M .Souza
 *
 * Color quantizator for a list of bitmaps.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "bitmap.h"
#include "quant.h"


static char *FileList[4096];     /* List of the files to be quantized. */
static unsigned int NFileList;   /* Number of files in the list */

static char *dir_16bpp, *dir_8bpp;
static char *pal_file;


/* Return 1 of the file name ends with `.spr' or `.spr.gz' */
static int is_spr_file(char *filename)
{
  int len;

  len = strlen(filename);
  if (len > 4 && strcmp(filename + len - 4, ".spr") == 0)
    return 1;
  if (len > 7 && strcmp(filename + len - 7, ".spr.gz") == 0)
    return 1;
  return 0;
}

static void print_options(char *prog_name)
{
  printf("%s options file ...\n"
	 "\n"
	 "options:\n"
	 "\n"
	 "  -h           Show this help\n"
	 "  -dir8 DIR    Output directory\n"
	 "  -dir16 DIR   Input directory (from where the files will be read)\n"
	 "  -ncolors N   Number of colors in the output (default: 255)\n"
	 "  -pal FILE    Output the palette to FILE (default: `out.pal')\n"
	 "\n", prog_name);
}

static int read_options(int argc, char *argv[],
			int *n_files, char **files, int *n_colors)
{
  int i, cur_file = 0;

  for (i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      if (strcmp(argv[i] + 1, "dir8") == 0) {
	if (argv[i + 1] == NULL) {
	  printf("%s: expected directory name for option `%s'\n",
		 argv[0], argv[i]);
	  return 1;
	}
	dir_8bpp = argv[++i];
      } else if (strcmp(argv[i] + 1, "dir16") == 0) {
	if (argv[i + 1] == NULL) {
	  printf("%s: expected directory name for option `%s'\n",
		 argv[0], argv[i]);
	  return 1;
	}
	dir_16bpp = argv[++i];
      } else if (strcmp(argv[i] + 1, "pal") == 0) {
	if (argv[i + 1] == NULL) {
	  printf("%s: expected directory name for option `%s'\n",
		 argv[0], argv[i]);
	  return 1;
	}
	pal_file = argv[++i];
      } else if (strcmp(argv[i] + 1, "ncolors") == 0) {
	if (argv[i + 1] == NULL) {
	  printf("%s: expected directory name for option `%s'\n",
		 argv[0], argv[i]);
	  return 1;
	}
	*n_colors = atoi(argv[++i]);
      } else if (strcmp(argv[i] + 1, "h") == 0) {
	print_options(argv[0]);
	return 1;
      }
    } else
      files[cur_file++] = argv[i];
  }

  *n_files = cur_file;
  return 0;
}

int main(int argc, char *argv[])
{
  unsigned int *GameColors = malloc(sizeof(unsigned int) * MAX_COLORS);
  unsigned int *GameColorsFreq = malloc(sizeof(unsigned int) * MAX_COLORS);
  unsigned int NGameColors, NFinalColors;
  ColorSpace *CSpace;

  dir_8bpp = dir_16bpp = NULL;
  pal_file = "out.pal";
  NFileList = 0;
  NFinalColors = 255;  /* Save a space in the palette for transparent pixels */

  if (read_options(argc, argv, &NFileList, FileList, &NFinalColors))
    return 1;

  if (NFileList == 0) {
    printf("No files given!\n");
    return 1;
  }
  if (dir_16bpp == NULL) {
    printf("Please specify the input directory\n");
    return 1;
  }
  if (dir_8bpp == NULL) {
    printf("Please specify the output directory\n");
    return 1;
  }

  NGameColors = ReadColors(NFileList, FileList, GameColors, GameColorsFreq);
  CSpace = QuantColors(NGameColors, GameColors, GameColorsFreq, NFinalColors);
  SaveColors(NFileList, FileList, CSpace, NGameColors, GameColors,
	     GameColorsFreq);

  free(GameColors);
  free(GameColorsFreq);

  return 0;
}

unsigned int ReadColors(unsigned int NFiles, char *Files[],
			unsigned int *Colors, unsigned int *Freq)
{
  XBITMAP *Frames[1024];
  unsigned int NFrames, NColors;
  unsigned int CurrFrame;
  unsigned int i, j;
  char FullFileName[1024];

  unsigned int *FileColorsFreq = malloc(sizeof(unsigned int) * MAX_COLORS);
  unsigned int CurrColor;

  for(i=0; i<MAX_COLORS; i++) Freq[i] = 0;

  printf("Reading files...\n");
  while(NFiles)
    {
      BMP_FONT *font;

      snprintf(FullFileName, sizeof(FullFileName), "%s/%s", dir_16bpp, Files[NFiles - 1]);

      /* printf("Reading `%s'...\n", FullFileName); */

      /* Read the file */
      if (is_spr_file(FullFileName)) {
	font = NULL;
	NFrames = read_xbitmaps(FullFileName, 1024, Frames);
	if (NFrames <= 0) {
	  printf("Error: can't read `%s'!\n", FullFileName);
	  exit(1);
	}
      } else {
	int c;

	font = read_bmp_font(FullFileName);
	if (font == NULL) {
	  printf("Error: can't read `%s'!\n", FullFileName);
	  exit(1);
	}

	for (c = 0; c < font->n; c++)
	  Frames[c] = font->bmp[c];
	NFrames = font->n;
      }

      for(i=0; i<MAX_COLORS; i++) FileColorsFreq[i] = 0;

      for (CurrFrame = NFrames; CurrFrame != 0; CurrFrame--)
	{
	  for(i=0; i<Frames[CurrFrame-1]->w; i++)
	    for(j=0; j<Frames[CurrFrame-1]->h; j++)
	      {
		CurrColor = ((int2bpp *)Frames[CurrFrame - 1]->line[j])[i];
		if(CurrColor != TRANSPARENT) FileColorsFreq[CurrColor]++;
	      }

	  /* Destroy the current frame */
	  if (font == NULL)
	    destroy_xbitmap(Frames[CurrFrame-1]);
	}
      if (font != NULL)
	destroy_bmp_font(font);

      /* Agora dividimos a frequencia das cores pelo numero de frames */
      if(NFrames) 
	for(i=0; i<MAX_COLORS; i++)
	  if(FileColorsFreq[i])
	    {
	      FileColorsFreq[i] /= NFrames;
	      /* So para ter certeza de que todas as cores terao
                 freq. pelo menos = 1 */
	      if(!FileColorsFreq[i]) FileColorsFreq[i] = 1;
	    }

      /* Agora vamos somar as frequencias `a tabela global de frequencias */
      for(i=0; i<MAX_COLORS; i++) Freq[i] += FileColorsFreq[i];

      NFiles--;
    }

  free(FileColorsFreq);

  /* Agora vamos converter para o formato apropriado onde Colors e' o
     vetor com o valor de cada cor e Freq e' o vetor correspondente
     com as frequencias das cores */
  NColors = 0;
  for(i=0; i<MAX_COLORS; i++)
    if(Freq[i])
      {
	Colors[NColors] = i;
	Freq[NColors] = Freq[i];
	NColors++;
      }

  for(i = NColors; i<MAX_COLORS; i++)
    { Colors[i] = 0; Freq[i] = 0; }

  return NColors;
}

ColorSpace *QuantColors(unsigned int NColors, unsigned int *Colors,
			unsigned int *Freq, unsigned int NFColors)
{
  unsigned char R[MAX_COLORS], G[MAX_COLORS], B[MAX_COLORS];
  unsigned int i, j;
  unsigned int maxR, minR, maxG, minG, maxB, minB;
  ColorSpace *CSpace, *LastCSpace;

  printf("Quantizing %d colors to %d colors...\n", NColors, NFColors);

  for(i=0; i < NColors; i++)
    {
      /* Convertemos tudo para RGB */
      R[i] = (Colors[i] >> 11) & 0x1F; R[i] = (R[i] << 3) | (R[i] >> 2);
      G[i] = (Colors[i] >>  5) & 0x3F; G[i] = (G[i] << 2) | (G[i] >> 4);
      B[i] = (Colors[i]      ) & 0x1F; B[i] = (B[i] << 3) | (B[i] >> 2);
    }

  LastCSpace = CSpace = (ColorSpace *) malloc(sizeof(ColorSpace));
  CSpace->Start = 0;
  CSpace->End = NColors;
  CSpace->next = NULL;

  /* Now, the moment everyone was waiting for! The quantization loop!!! */
  for(i=0; i<NFColors-1; i++)
    {
      /* First, let's find out the biggest axis of the current color space */
      maxR = minR = R[CSpace->Start];
      maxG = minG = G[CSpace->Start];
      maxB = minB = B[CSpace->Start];

      for(j=CSpace->Start; j<CSpace->End; j++)
	{
	  if(R[j] > maxR) maxR = R[j]; else if(R[j] < minR) minR = R[j];
	  if(G[j] > maxG) maxG = G[j]; else if(G[j] < minG) minG = G[j];
	  if(B[j] > maxB) maxB = B[j]; else if(B[j] < minB) minB = B[j];
	}

      if ((maxR - minR) > (maxG - minG))
	if ((maxR - minR) > (maxB - minB))
	  Sort(CSpace, R, G, B, Colors, Freq);
	else
	  Sort(CSpace, B, G, R, Colors, Freq);
      else if ((maxB - minB) > (maxG - minG))
	Sort(CSpace, B, G, R, Colors, Freq);
      else
	Sort(CSpace, G, R, B, Colors, Freq);

      if((CSpace->End - CSpace->Start) > 1)
	{
	  int b;

	  b = ((CSpace->End - CSpace->Start)/2) + CSpace->Start;
	  LastCSpace->next = (ColorSpace *) malloc(sizeof(ColorSpace));
	  LastCSpace->next->Start = b;
	  LastCSpace->next->End = CSpace->End;
	  CSpace->End = b;
	  LastCSpace->next->next = CSpace;
	  LastCSpace = CSpace;
	  CSpace = CSpace->next;
	  LastCSpace->next = NULL;
	}
      else
	{
	  LastCSpace->next = CSpace;
	  LastCSpace = CSpace;
	  CSpace = CSpace->next;
	  LastCSpace->next = NULL;
	}
    }

  return CSpace;
}

static unsigned char *V;

static int do_compare(const void *a, const void *b)
{
  return V[*(int *)a] > V[*(int *)b];
}

int Sort(ColorSpace *CSpace, unsigned char *V1, unsigned char *V2,
	 unsigned char *V3, unsigned int *V4, unsigned int *V5)
{
  unsigned int i, v[MAX_COLORS], tmp[MAX_COLORS];

  V = V1;

  for (i = CSpace->Start; i < CSpace->End; i++)
    v[i] = i;

  qsort(v + CSpace->Start, CSpace->End - CSpace->Start, sizeof(unsigned int),
	do_compare);

  for (i = CSpace->Start; i < CSpace->End; i++) tmp[i] = V1[v[i]];
  for (i = CSpace->Start; i < CSpace->End; i++) V1[i] = tmp[i];

  for (i = CSpace->Start; i < CSpace->End; i++) tmp[i] = V2[v[i]];
  for (i = CSpace->Start; i < CSpace->End; i++) V2[i] = tmp[i];

  for (i = CSpace->Start; i < CSpace->End; i++) tmp[i] = V3[v[i]];
  for (i = CSpace->Start; i < CSpace->End; i++) V3[i] = tmp[i];

  for (i = CSpace->Start; i < CSpace->End; i++) tmp[i] = V4[v[i]];
  for (i = CSpace->Start; i < CSpace->End; i++) V4[i] = tmp[i];

  for (i = CSpace->Start; i < CSpace->End; i++) tmp[i] = V5[v[i]];
  for (i = CSpace->Start; i < CSpace->End; i++) V5[i] = tmp[i];

  return 0;
}

XBITMAP *reduce_colors(XBITMAP *bmp, unsigned char *Map)
{
  XBITMAP *out;
  int x, y;
  int2bpp *src;
  unsigned char *dest;

  out = create_xbitmap(bmp->w, bmp->h, 1);
  if (out == NULL)
    return NULL;

  for (y = 0; y < bmp->h; y++) {
    dest = out->line[y];
    src = (int2bpp *) bmp->line[y];
    for (x = 0; x < bmp->w; x++) {
      if (*src == TRANSPARENT)
	*dest++ = 0;
      else
	*dest++ = Map[*src];
      src++;
    }
  }
  return out;
}

void SaveColors(unsigned int NFiles, char **Name, ColorSpace *LCSpace,
		unsigned int NColors, unsigned int *Colors, unsigned int *Freq)
{
  XBITMAP *in[1024], *out[1024];
  char filename[256];
  ColorSpace *CSpace;
  unsigned int i, j, cur_color, n;
  unsigned char R[MAX_COLORS], G[MAX_COLORS], B[MAX_COLORS];
  unsigned char pal[768];
  unsigned char Map[MAX_COLORS];

  printf("Writting bitmaps...\n");

  for (i = 0; i < NColors; i++)
    {
      /* Convert everything to RGB */
      R[i] = (Colors[i] >> 11) & 0x1F; R[i] = (R[i] << 3) | (R[i] >> 2);
      G[i] = (Colors[i] >>  5) & 0x3F; G[i] = (G[i] << 2) | (G[i] >> 4);
      B[i] = (Colors[i]      ) & 0x1F; B[i] = (B[i] << 3) | (B[i] >> 2);
    }

  for (i = 0; i < MAX_COLORS; i++)
    Map[i] = 1;

  pal[0] = pal[1] = pal[2] = 0;
  cur_color = 1;
  for (CSpace = LCSpace; CSpace != NULL; CSpace = CSpace->next) {
    int r, g, b, s;

    r = g = b = s = 0;
    for (i = CSpace->Start; i < CSpace->End; i++) {
      Map[Colors[i]] = cur_color;
      s += Freq[i];
      r += Freq[i] * R[i];
      g += Freq[i] * G[i];
      b += Freq[i] * B[i];
    }
    pal[3 * cur_color] = r / s;
    pal[3 * cur_color + 1] = g / s;
    pal[3 * cur_color + 2] = b / s;
    cur_color++;
  }

  {
    FILE *f;
 
    snprintf(filename, sizeof(filename), "%s/%s", dir_8bpp, pal_file);
    if ((f = fopen(filename, "wb")) == NULL) {
      printf("Error: can't write `%s'\n", filename);
      exit(1);
    }
    fwrite(pal, 3, 256, f);
    fclose(f);
  }

  for (i = 0; i < NFiles; i++) {
    BMP_FONT *font;

    snprintf(filename, sizeof(filename), "%s/%s", dir_16bpp, Name[i]);

    /* Read the input file */
    if (is_spr_file(filename)) {
      font = NULL;
      n = read_xbitmaps(filename, 1024, in);
      if (n <= 0) {
	printf("Error: can't read `%s'\n", filename);
	exit(1);
      }
    } else {
      int c;

      font = read_bmp_font(filename);
      if (font == NULL) {
	printf("Error: can't read `%s'\n", filename);
	exit(1);
      }
      for (c = 0; c < font->n; c++)
	in[c] = font->bmp[c];
      n = font->n;
    }

    /* Convert to 8bpp, reducing the colors */
    for (j = 0; j < n; j++)
      if ((out[j] = reduce_colors(in[j], Map)) == NULL) {
	printf("Out of memory for frame %d of `%s'\n", j, filename);
	exit(1);
      }

    /* Write the output */
    snprintf(filename, sizeof(filename), "%s/%s", dir_8bpp, Name[i]);
    /* printf("Writing `%s'\n", filename); */

    if (font == NULL) {
      write_xbitmaps(filename, n, out);
      for (j = 0; j < n; j++) {
	destroy_xbitmap(in[j]);
	destroy_xbitmap(out[j]);
      }
    } else {
      BMP_FONT *out_font;
      int c;

      out_font = malloc(sizeof(BMP_FONT));
      out_font->n = font->n;
      for (c = 0; c < font->n; c++)
	out_font->bmp[c] = out[c];
      write_bmp_font(filename, out_font);
      destroy_bmp_font(font);
      destroy_bmp_font(out_font);
    }
  }

}
