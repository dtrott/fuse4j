#ifndef _FUSE_CALLBACK_H_
#define _FUSE_CALLBACK_H_

#include "javafs.h"
#include "javafs_bindings.h"

extern jclass_fuse_FuseContext *FuseContext;


int alloc_classes(JNIEnv *env);
void free_classes(JNIEnv *env);


#endif
