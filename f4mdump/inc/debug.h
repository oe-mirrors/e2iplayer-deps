#ifndef SULGE_F4M_DEBUG
#define SULGE_F4M_DEBUG

#include <cstdio>
#include <assert.h>

//#define DEBUG

#ifdef DEBUG
#define ASSERT assert
#define printDBG(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( false )
#else
#define ASSERT {}
#define printDBG(...) do{ } while ( false )
#endif

#define printExc(...) do{ fprintf( stdout, __VA_ARGS__ ); } while( false )

#define printInf(...) do{ fprintf( stderr, __VA_ARGS__ ); } while( false )

#endif /* SULGE_F4M_DEBUG */