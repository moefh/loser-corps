/* rect.h */

#ifndef RECT_H_FILE
#define RECT_H_FILE

#ifndef INLINE
#ifdef __GNUC__
#define INLINE __inline__
#else
#define INLINE
#endif
#endif

typedef struct RECT {
  int x, y;
  int w, h;
} RECT;

/* Return in `c' the interception of the rectangles `a' and `b' (if any).
 * If there's interception, returns 1; otherwise, returns 0 */
static INLINE int rect_interception(RECT *a, RECT *b, RECT *c)
{
  c->x = MAX(a->x, b->x);
  c->y = MAX(a->y, b->y);
  c->w = MIN(a->x + a->w, b->x + b->w) - c->x;
  c->h = MIN(a->y + a->h, b->y + b->h) - c->y;

  return (c->w > 0 && c->h > 0);
}

#endif /* RECT_H_FILE */
