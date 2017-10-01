/* inline_asm.h */

#ifndef INLINE_ASM_H
#define INLINE_ASM_H


/* This is like `memcpy()', but writes two
 * bytes in `dest' for each byte in `src'. */

#define double_memcpy(dest, src, n)				\
  __asm__ __volatile__("cld\n"					\
		       "1:\n\t"					\
		       "lodsb\n\t"				\
		       "movb %%al,%%ah\n\t"			\
		       "stosw\n\t"				\
		       "loop 1b"				\
		       : /* no outputs */			\
		       : "D" (dest), "S" (src), "c" (n)		\
		       : "ax", "esi", "edi", "ecx", "memory");

#define double_memcpy16(dest, src, n)					\
  __asm__ __volatile__("cld\n"						\
		       "1:\n\t"						\
		       "movl (%%esi),%%ebx\n\t"				\
		       "incl %%esi\n\t"					\
		       "incl %%esi\n\t"					\
		       "incl %%esi\n\t"					\
		       "incl %%esi\n\t"					\
		       "movw %%bx,%%ax\n\t"				\
		       "shll $16,%%eax\n\t"				\
		       "movw %%bx,%%ax\n\t"				\
		       "stosl\n\t"					\
		       "shrl $16,%%ebx\n\t"				\
		       "movw %%bx,%%ax\n\t"				\
		       "shll $16,%%eax\n\t"				\
		       "movw %%bx,%%ax\n\t"				\
		       "stosl\n\t"					\
		       "loop 1b"					\
		       : /* no outputs */				\
		       : "D" (dest), "S" (src), "c" ((n)/2)		\
		       : "ax", "esi", "edi", "bx", "ecx", "memory");


#define single_memcpy(dest, src, n)			\
  __asm__ __volatile__ ("cld\n\t"			\
			"shrl $1,%%ecx\n\t"		\
			"jnc 1f\n\t"			\
			"movsb\n"			\
			"1:\tshrl $1,%%ecx\n\t"		\
			"jnc 2f\n\t"			\
			"movsw\n"			\
			"2:\trep\n\t"			\
			"movsl"				\
			: /* no output */		\
			:"D" (dest), "S" (src), "c" (n)	\
			:"cx","di","si","memory");


/* Transparent memcpy(): copy only non-zero bytes */
#define tr_memcpy(dest, src, n)					\
  __asm__ __volatile__("cld\n"					\
		       "1:\n\t"					\
		       "lodsb\n\t"				\
		       "orb %%al, %%al\n\t"			\
		       "jz 2f\n\t"				\
		       "movb %%al, (%%edi)\n"			\
		       "2:\n\t"					\
		       "incl %%edi\n\t"				\
		       "loop 1b"				\
		       : /* No outputs */			\
		       : "D" (dest), "S" (src), "c" (n)		\
		       : "esi", "edi", "al", "ecx", "memory");

#endif /* INLINE_ASM_H */
