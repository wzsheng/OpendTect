#ifndef plftypes_h
#define plftypes_h

/*
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Contents:	Platform dependent types
 RCS:		$Id: plftypes.h,v 1.3 2004-12-17 12:38:43 bert Exp $
________________________________________________________________________

*/

#include "plfdefs.h"

#if defined(__sun__) || defined(__sgi__)
# include <sys/types.h>
#else
# ifndef __win__
#  include <stdint.h>
# endif
#endif

/* 16 bits short is standard. Only use to emphasise the 16-bitness */
#define int16 short
#define uint16 unsigned short

/* 32 bits int is standard. Only use to emphasise the 32-bitness */
#define int32 int
#define uint32 unsigned int

/* 64 bits is int64_t on UNIX. The definition is in various header files. */
#ifndef __win__
# define int64 int64_t
# define uint64 uint64_t
#else
# define int64 TODO_AREND_see_Basic_plftypes_h
# define uint64 TODO_AREND_see_Basic_plftypes_h
#endif


#endif
