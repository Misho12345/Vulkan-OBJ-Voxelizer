#pragma once

#ifdef NDEBUG
#define DEBUG_ONLY(X) (void)0
#else
#define DEBUG_ONLY(X) X
#endif
