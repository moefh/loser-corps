/* internal.h
 *
 * XDialogs internal functions and variables.
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#ifdef __GNUC__
#define INLINE inline
#else
#define INLINE
#endif

/* Double click time, in microsseconds */
#define DCLICK_TIME  300000

/* Events we want to receive notification about */
#define EV_MASK KeyPressMask|ButtonPressMask|ExposureMask|\
                StructureNotifyMask|FocusChangeMask|EnterWindowMask|\
                LeaveWindowMask|PointerMotionMask

#ifndef MIN
#define MIN(a,b)  (((a) < (b)) ? (a) : (b))
#define MAX(a,b)  (((a) > (b)) ? (a) : (b))
#endif

#ifndef ABS
#define ABS(a)      (((a) >= 0) ? (a) : -(a))
#endif

#define DIST2(a,b)   ((a)*(a) + (b)*(b))

#endif /* INTERNAL_H */
