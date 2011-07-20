#ifndef __UTIL_H__
#define __UTIL_H__

#define ADDARRAY(a, e) \
	do { \
	if ((a ## num) >= (a ## max)) { \
		if (!(a ## max)) \
			(a ## max) = 16; \
		else \
			(a ## max) *= 2; \
		(a) = realloc((a), (a ## max)*sizeof(*(a))); \
	} \
	(a)[(a ## num)++] = (e); \
	} while(0)

#define FINDARRAY(a, tmp, pred)				\
	({							\
		int __i;					\
								\
		for (__i = 0; __i < (a ## num); __i++) {	\
			tmp = (a)[__i];				\
			if (pred)				\
				break;				\
		}						\
								\
		tmp = ((pred) ? tmp : NULL);			\
	})

#define min(a,b)				\
	({					\
		typeof (a) _a = (a);		\
		typeof (b) _b = (b);		\
		_a < _b ? _a : _b;		\
	})

#define max(a,b)				\
	({					\
		typeof (a) _a = (a);		\
		typeof (b) _b = (b);		\
		_a > _b ? _a : _b;		\
	})

#endif
