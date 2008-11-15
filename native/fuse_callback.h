#ifndef _FUSE_CALLBACK_H_
#define _FUSE_CALLBACK_H_

#include "javafs.h"

extern JavaVM *vm;
extern JNIEnv *mainEnv;

extern jobject threadGroup;
extern jclass_fuse_FuseContext *FuseContext;

extern struct fuse_operations javafs_oper;


int alloc_classes(JNIEnv *env);
void free_classes(JNIEnv *env);


#endif
