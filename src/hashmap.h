/*
 * Generic hashmap manipulation functions
 *
 * Originally by Elliot C Back - http://elliottback.com/wp/hashmap-implementation-in-c/
 *
 * Modified by Pete Warden to fix a serious performance problem, support strings as keys
 * and removed thread synchronization - http://petewarden.typepad.com
 
 * https://github.com/petewarden/c_hashmap
 */
#ifndef __HASHMAP_H__
#define __HASHMAP_H__

#include "types.h"

s32 hashmap_hash_s32(const char* keystring);

#endif /* __HASHMAP_H__ */
