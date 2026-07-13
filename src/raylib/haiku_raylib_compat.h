#pragma once

#ifdef __HAIKU__
#include <pthread.h>

// Haiku does not provide pthread_attr_setinheritsched(). raylib 6.0's bundled
// miniaudio uses it only while trying to adjust thread scheduling attributes,
// so a no-op keeps the default scheduler behavior.
#ifndef pthread_attr_setinheritsched
#define pthread_attr_setinheritsched(attr, inheritsched) (0)
#endif
#endif
