#ifndef __UTIL_H__
#define __UTIL_H__

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
