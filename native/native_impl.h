#ifndef _NATIVE_IMPL_H_
#define _NATIVE_IMPL_H_

#include "javafs.h"

extern jobject fuseFS;

void free_fuseFS(JNIEnv *env);

#endif
