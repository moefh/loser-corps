/* misc.c */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "mapedit.h"


/* Create a shared image */
IMAGE *create_image(Display *disp, int w, int h, int bpp)
{
  IMAGE *i;

  i = (IMAGE *) malloc(sizeof(IMAGE));
  if (i == NULL)
    return NULL;

#ifdef USE_SHM
  if (use_shm) {
    Screen *scr;

    scr = DefaultScreenOfDisplay(disp);
    i->img = XShmCreateImage(disp, DefaultVisualOfScreen(scr),
			     8*bpp, ZPixmap, NULL, &i->info, w, h);

    if (i->img == NULL) {
      free(i);
      return NULL;
    }
    i->info.shmid = shmget(IPC_PRIVATE,
			   i->img->bytes_per_line * i->img->height,
			   IPC_CREAT | 0777);
    if (i->info.shmid < 0) {
      free(i);
      return NULL;
    }
    i->img->data = i->info.shmaddr = shmat(i->info.shmid, 0, 0);
    if (i->img->data == NULL) {
      shmctl(i->info.shmid, IPC_RMID, 0);
      free(i);
      return NULL;
    }
    i->info.readOnly = False;
    XShmAttach(disp, &i->info);
    i->is_shared = 1;
    return i;
  }
  i->is_shared = 0;
#endif /* USE_SHM */
  i->img = XCreateImage(disp,
			DefaultVisualOfScreen(DefaultScreenOfDisplay(disp)),
			8*bpp, ZPixmap, 0, NULL, w, h, 8*bpp, 0);
  if (i->img == NULL) {
    printf("XCreateImage() returns NULL (bpp=%d)\n", bpp);
    return NULL;
  }
  i->img->data = (char *) malloc(i->img->height * i->img->bytes_per_line);
  if (i->img->data == NULL) {
    free(i->img);
    return NULL;
  }
  return i;
}

/* Free a shared image */
void destroy_image(Display *disp, IMAGE *img)
{
#ifdef USE_SHM
  if (img->is_shared) {
    XShmDetach(disp, &img->info);
    if (img->info.shmaddr != 0)
      shmdt(img->info.shmaddr);
    if (img->info.shmid >= 0)
      shmctl(img->info.shmid, IPC_RMID, 0);
  } else
#endif /* USE_SHM */
    XDestroyImage(img->img);
  free(img);
}

void display_image(WINDOW *win, int x, int y, IMAGE *image,
		   int img_x, int img_y, int w, int h)
{
#if USE_SHM
  if (image->is_shared)
    XShmPutImage(win->disp, win->window, win->gc,
		 image->img, img_x, img_y, x, y, w, h, 0);
  else
#endif /* USE_SHM */
    x_put_image(win->disp, win->window, win->gc,
                image->img, img_x, img_y, x, y, w, h);
}

static XImage *tmp_image;

static void init_x_put_image(Display *disp)
{
  Screen *screen = DefaultScreenOfDisplay(disp);
  int depth = DefaultDepth(disp, DefaultScreen(disp));

  tmp_image = XCreateImage(disp,
                           DefaultVisualOfScreen(DefaultScreenOfDisplay(disp)),
                           depth, ZPixmap, 0, NULL, screen->width, screen->height, (depth == 24 || depth == 32) ? 32 : 16, 0);
  if (tmp_image == NULL) {
    printf("XCreateImage() returns NULL (depth=%d, w=%d, h=%d)\n", depth, screen->width, screen->height);
    return;
  }
  tmp_image->data = malloc(screen->height * tmp_image->bytes_per_line);
  printf("Created tmp image: depth=%d, bitmap_unit=%d, bitmap_pad=%d\n", tmp_image->depth, tmp_image->bitmap_unit, tmp_image->bitmap_pad);
}

static void copy_img_line(void *dst, int dst_depth, void *src, int src_depth, int num_pixels)
{
  if (dst_depth == src_depth) {
    memcpy(dst, src, num_pixels * dst_depth);
    return;
  }

  if (dst_depth != 3 && dst_depth != 4) {
    return;
  }
  
  switch (src_depth) {
  case 2: {
    unsigned short *s = src;
    unsigned char *d = dst;
    for (int p = 0; p < num_pixels; p++) {
      *d++ = (*s & 0x1f) << 3;
      *d++ = ((*s >> 5) & 0x3f) << 2;
      *d++ = (*s >> 11) << 3;
      s++;
      if (dst_depth == 4)
        *d++ = 0xff;
    }
    break;
  }

  case 3:
  case 4: {
    unsigned char *s = src;
    unsigned char *d = dst;
    for (int p = 0; p < num_pixels; p++) {
      *d++ = *s++;
      *d++ = *s++;
      *d++ = *s++;
      if (dst_depth == 4)
        *d++ = 0;
      if (src_depth == 4)
        s++;
    }
  }

  default:
    printf("ERROR: unsupported dest depth: %d\n", dst_depth);
  }
  
}

void x_put_image(Display *disp, Drawable d, GC gc, XImage *image, int src_x, int src_y, int dest_x, int dest_y, unsigned int width, unsigned int height)
{
  if (tmp_image == NULL)
    init_x_put_image(disp);

  // copy image to tmp_image
  XImage *src_img = image;
  XImage *dst_img = tmp_image;
  char *src = src_img->data + src_y * src_img->bytes_per_line + src_x * (src_img->bitmap_pad/8);
  char *dst = dst_img->data;
  for (int y = 0; y < height; y++) {
    copy_img_line(dst, dst_img->bitmap_pad/8, src, src_img->bitmap_pad/8, width);
    src += src_img->bytes_per_line;
    dst += dst_img->bytes_per_line;
  }
  
  // draw tmp_image
  XPutImage(disp, d, gc, tmp_image, 0, 0, dest_x, dest_y, width, height);
}
