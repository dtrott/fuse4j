#include "native_impl.h"
#include "fuse_callback.h"
#include "util.h"

/*
 * Class:     fuse_FuseMount
 * Method:    mount
 * Signature: ([Ljava/lang/String;Lfuse/FuseFS;Ljava/lang/ThreadGroup;)V
 */
JNIEXPORT void JNICALL Java_fuse_FuseMount_mount(JNIEnv *env, jclass class, jobjectArray jArgs, jobject jFuseFS, jobject jThreadGroup)
{
   if (!((*env)->GetJavaVM(env, &vm)))
   {
      mainEnv = env;
      int i;
      int n = (*env)->GetArrayLength(env, jArgs);
      int fuseArgc = n + 1;
      char *fuseArgv[fuseArgc];

      // fake 1st argument to be the name of executable
      fuseArgv[0] = "javafs";

      // convert String[] jArgs -> char *fuseArgv[];
      for (i = 0; i < n; i++)
      {
         jstring jArg = (*env)->GetObjectArrayElement(env, jArgs, i);
         const char *arg = (*env)->GetStringUTFChars(env, jArg, NULL);
         char *fuseArg = (char *)malloc(strlen(arg) + 1);
         strcpy(fuseArg, arg);
         (*env)->ReleaseStringUTFChars(env, jArg, arg);
         (*env)->DeleteLocalRef(env, jArg);

         fuseArgv[i + 1] = fuseArg;
      }

      /*
      printf("%d fuse arguments:", fuseArgc);
      for (i = 0; i < fuseArgc; i++)
         printf(" %s", fuseArgv[i]);
      printf("\n");
      */

      if (alloc_classes(env))
      {
         if (retain_fuseFS(env, jFuseFS))
         {
            if (retain_threadGroup(env, jThreadGroup))
            {
               jint jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.init);
               exception_check_jerrno(env, &jerrno);

               if (jerrno == 0) {
                   // main loop
                   fuse_main(fuseArgc, fuseArgv, &javafs_oper, NULL);

                   jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.destroy);
                   exception_check_jerrno(env, &jerrno);
               }

               // cleanup
               free_threadGroup(env);
            }

            // cleanup
            free_fuseFS(env);
         }

         // cleanup
         free_classes(env);
      }

      // free char *fuseArgv[] strings
      for (i = 1; i < fuseArgc; i++)
      {
         free(fuseArgv[i]);
      }

      vm = NULL;
      mainEnv = NULL;
   }
}

/*
 * Class:     fuse_FuseContext
 * Method:    fillInFuseContext
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_fuse_FuseContext_fillInFuseContext(JNIEnv *env, jobject jContext)
{
   struct fuse_context *context = fuse_get_context();

   (*env)->SetIntField(env, jContext, FuseContext->field.uid, (jint)(context->uid));
   (*env)->SetIntField(env, jContext, FuseContext->field.gid, (jint)(context->gid));
   (*env)->SetIntField(env, jContext, FuseContext->field.pid, (jint)(context->pid));
}


/*
 * Class:     fuse_FuseFSFillDir
 * Method:    fill
 * Signature: (Ljava/nio/ByteBuffer;JIJJJ)Z
 */
JNIEXPORT jboolean JNICALL Java_fuse_FuseFSFillDir_fill
  (JNIEnv *env, jobject jFillDir, jobject jName, jlong inode, jint mode, jlong nextOffset, jlong buf, jlong fillDir)
{
   // cast jlong (64 bit signed integer) to function pointer
   fuse_fill_dir_t fill_dir = (fuse_fill_dir_t) fillDir;

   const char *name = (const char *) (*env)->GetDirectBufferAddress(env, jName);

   struct stat stbuf;
   stbuf.st_ino = (ino_t) inode;
   stbuf.st_mode = (mode_t) mode;

   int retval = fill_dir((void *)buf, name, &stbuf, (off_t) nextOffset);

   return (retval == 0)? JNI_TRUE : JNI_FALSE;
}

static int RegisterMethod(JNIEnv *env, jclass cls, char *name, char *signature, void *fnPtr)
{
    JNINativeMethod nm;

    while(1)
    {
        nm.name = name;
        nm.signature = signature;
        nm.fnPtr = fnPtr;

        (*env)->RegisterNatives(env, cls, &nm, 1);
        if ((*env)->ExceptionCheck(env)) break;

        return 1;
    }

    if ((*env)->ExceptionCheck(env))
    {
        (*env)->ExceptionDescribe(env);
    }

   return 0;
}

static int LoadClassAndRegisterMethod(JNIEnv *env, char *class_name, char *name, char *signature, void *fnPtr)
{
    jclass class;
    int result;

    while(1)
    {
        class = (*env)->FindClass(env, class_name);
        if ((*env)->ExceptionCheck(env)) break;

        result = RegisterMethod(env, class, name, signature, fnPtr);

        if (class != NULL) {
            (*env)->DeleteLocalRef(env, class);
        }

        return result;
    }

    if ((*env)->ExceptionCheck(env)) {
        // error handler
        (*env)->ExceptionDescribe(env);
        (*env)->ExceptionClear(env);
    }

    if (class != NULL) {
        (*env)->DeleteLocalRef(env, class);
    }

    return 0;
}

int RegisterNativeMethods(JNIEnv *env)
{
    while(1)
    {
        if (!RegisterMethod            (env,  FuseContext->class,  "fillInFuseContext", "()V",              Java_fuse_FuseContext_fillInFuseContext)) break;
        if (!LoadClassAndRegisterMethod(env, "fuse/FuseFSFillDir", "fill", "(Ljava/nio/ByteBuffer;JIJJJ)Z", Java_fuse_FuseFSFillDir_fill)) break;

        return 1;
    }
    return 0;
}
