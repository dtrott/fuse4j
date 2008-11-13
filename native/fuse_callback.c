/**
 *   FUSE-J: Java bindings for FUSE (Filesystem in Userspace by Miklos Szeredi (mszeredi@inf.bme.hu))
 *
 *   Copyright (C) 2003 Peter Levart (peter@select-tech.si)
 *
 *   This program can be distributed under the terms of the GNU LGPL.
 *   See the file COPYING.LIB
 */

#include "javafs.h"

//
// JVM allocation/deallocation

static JavaVM *vm;
static JNIEnv *mainEnv;

static jclass_fuse_FuseGetattr *FuseGetattr;
static jclass_fuse_FuseFSDirEnt *FuseFSDirEnt;
static jclass_fuse_FuseFSDirFiller *FuseFSDirFiller;
static jclass_fuse_FuseStatfs *FuseStatfs;
static jclass_fuse_FuseOpen *FuseOpen;
static jclass_fuse_FuseSize *FuseSize;
static jclass_fuse_FuseContext *FuseContext;
static jclass_java_nio_ByteBuffer *ByteBuffer;
static jclass_fuse_FuseFS *FuseFS;

static jobject fuseFS = NULL;
static jobject threadGroup = NULL;

static void create_file_handle(struct fuse_file_info *ffi, jobject ob)
{
    ffi->fh = (uint64_t) ob;
}

static jobject read_file_handle(struct fuse_file_info *ffi)
{
    return (jobject) ffi->fh;
}

/*
static int retain_fuseFS(JNIEnv *env, jobject util)
{
   fuseFS = (*env)->NewGlobalRef(env, util);

   if ((*env)->ExceptionCheck(env))
   {
      (*env)->ExceptionDescribe(env);
      return 0;
   }

   return 1;
}
*/

static void free_fuseFS(JNIEnv *env)
{
   if (fuseFS != NULL) { (*env)->DeleteGlobalRef(env, fuseFS); fuseFS = NULL; }
}


/*
static int retain_threadGroup(JNIEnv *env, jobject util)
{
   threadGroup = (*env)->NewGlobalRef(env, util);

   if ((*env)->ExceptionCheck(env))
   {
      (*env)->ExceptionDescribe(env);
      return 0;
   }

   return 1;
}
*/

/*
static void free_threadGroup(JNIEnv *env)
{
   if (threadGroup != NULL) { (*env)->DeleteGlobalRef(env, threadGroup); threadGroup = NULL; }
}
*/

static int alloc_filesystem(JNIEnv *env, char *filesystemClassName)
{
   jclass fsClass = NULL;
   jobject util = NULL;
   jmethodID fsConstructorID;

   while (1)
   {
      fsClass = (*env)->FindClass(env, filesystemClassName);
      if ((*env)->ExceptionCheck(env)) break;

      fsConstructorID = (*env)->GetMethodID(env, fsClass, "<init>", "()V");
      if ((*env)->ExceptionCheck(env)) break;

      util = (*env)->NewObject(env, fsClass, fsConstructorID);
      if ((*env)->ExceptionCheck(env)) break;

      fuseFS = (*env)->NewGlobalRef(env, util);
      if ((*env)->ExceptionCheck(env)) break;
      
      TRACE("FILE SYSTEM ALLOC OK");

      return 1; /* success */
   }

   TRACE("FILE SYSTEM ALLOC FAILED");

   // error handler

   if ((*env)->ExceptionCheck(env))
   {
      (*env)->ExceptionDescribe(env);
   }

   free_fuseFS(env);

   if (util != NULL) (*env)->DeleteLocalRef(env, util);
   if (fsClass != NULL) (*env)->DeleteLocalRef(env, fsClass);

   return 0;
}


static void free_classes(JNIEnv *env)
{
   if (FuseGetattr != NULL)     { free_jclass_fuse_FuseGetattr(env, FuseGetattr);         FuseGetattr = NULL; }
   if (FuseFSDirEnt != NULL)    { free_jclass_fuse_FuseFSDirEnt(env, FuseFSDirEnt);       FuseFSDirEnt = NULL; }
   if (FuseFSDirFiller != NULL) { free_jclass_fuse_FuseFSDirFiller(env, FuseFSDirFiller); FuseFSDirFiller = NULL; }
   if (FuseStatfs != NULL)      { free_jclass_fuse_FuseStatfs(env, FuseStatfs);           FuseStatfs = NULL; }
   if (FuseOpen != NULL)        { free_jclass_fuse_FuseOpen(env, FuseOpen);               FuseOpen = NULL; }
   if (FuseSize != NULL)        { free_jclass_fuse_FuseSize(env, FuseSize);               FuseSize = NULL; }
   if (FuseContext != NULL)     { free_jclass_fuse_FuseContext(env, FuseContext);         FuseContext = NULL; }
   if (ByteBuffer != NULL)      { free_jclass_java_nio_ByteBuffer(env, ByteBuffer);       ByteBuffer = NULL; }
   if (FuseFS != NULL)          { free_jclass_fuse_FuseFS(env, FuseFS);                   FuseFS = NULL; }
}


static int alloc_classes(JNIEnv *env)
{
   while (1)
   {
      if (!(FuseGetattr     = alloc_jclass_fuse_FuseGetattr(env))) break;
      if (!(FuseFSDirEnt    = alloc_jclass_fuse_FuseFSDirEnt(env))) break;
      if (!(FuseFSDirFiller = alloc_jclass_fuse_FuseFSDirFiller(env))) break;
      if (!(FuseStatfs      = alloc_jclass_fuse_FuseStatfs(env))) break;
      if (!(FuseOpen        = alloc_jclass_fuse_FuseOpen(env))) break;
      if (!(FuseSize        = alloc_jclass_fuse_FuseSize(env))) break;
      if (!(FuseContext     = alloc_jclass_fuse_FuseContext(env))) break;
      if (!(ByteBuffer      = alloc_jclass_java_nio_ByteBuffer(env))) break;
      if (!(FuseFS          = alloc_jclass_fuse_FuseFS(env))) break;

      return 1;
   }

   // error handler

   if ((*env)->ExceptionCheck(env))
   {
      (*env)->ExceptionDescribe(env);
   }

   free_classes(env);

   return 0;
}


static void free_JVM(JNIEnv *env)
{
   if (vm != NULL)
   {
      (*vm)->DestroyJavaVM(vm);
      vm = NULL;
      mainEnv = NULL;
   }
}


static JNIEnv *alloc_JVM(int argc, char *argv[])
{
   JavaVMInitArgs vm_args;
   int n = argc + 3;
   JavaVMOption options[n];
   int i;
   jint res;

   for (i = 0; i < argc; i++)
      options[i].optionString = argv[i];

   /* options[i++].optionString = "-verbose:jni";                        print JNI-related messages */
   options[i++].optionString = "-Xint";
   options[i++].optionString = "-Xrs";
   options[i++].optionString = "-Xcheck:jni";

   vm_args.version = JNI_VERSION_1_4;
   vm_args.options = options;
   vm_args.nOptions = n;
   vm_args.ignoreUnrecognized = 0;

   res = JNI_CreateJavaVM(&vm, (void **)&mainEnv, &vm_args);
   if (res < 0)
   {
      WARN("Can't create Java VM");
      return NULL;
   }

   TRACE("created JVM @ %p", vm);

   return mainEnv;
}


//
// attaching/detachnig current thread to/from JVM

static JNIEnv *get_env()
{
   JNIEnv *env;
   JavaVMAttachArgs args;

   args.version = JNI_VERSION_1_4;
   args.name = NULL;
   args.group = threadGroup;

   // a GCJ 4.0 bug workarround (supplied by Alexander Boström <abo@stacken.kth.se>)
   if ((*vm)->GetEnv(vm, (void**)&env, args.version) == JNI_OK)
      return env;

   TRACE("will attach thread");

   // attach thread as daemon thread so that JVM can exit after unmounting the fuseFS
   (*vm)->AttachCurrentThreadAsDaemon(vm, (void**)&env, (void*)&args);

   TRACE("did attach thread to env: %p", env);

   return env;
}

static void release_env(JNIEnv *env)
{
   if (env == mainEnv)
   {
      TRACE("will NOT detach main thread from env: %p", env);
   }
   else
   {
      TRACE("will NOT detach thread from env: %p", env);

      // Currently native threads are attached to JVM as daemon threads so we don't need to
      // detach them at return from a Java method call. It is in fact beter not to detach them
      // since then every new Java call would need to atach new Java Thread until max. number of
      // Java threads is exhausted.
      //
      //TRACE("will detach thread from env: %p", env);
      //(*vm)->DetachCurrentThread(vm);
      //TRACE("did detach thread");
   }
}

static jint exception_check_jerrno(JNIEnv *env, jint *jerrno)
{
   if ((*env)->ExceptionCheck(env))
   {
      (*env)->ExceptionDescribe(env);
      (*env)->ExceptionClear(env);
      if (*jerrno == 0)
      {
         *jerrno = EFAULT;
      }
   }

   return *jerrno;
}


//
// javafs API functions

static int javafs_getattr(const char *path, struct stat *stbuf)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jGetattr = NULL;
   jint jerrno = 0;

    TRACE("Start: javafs_getattr");

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jGetattr = (*env)->NewObject(env, FuseGetattr->class, FuseGetattr->constructor.new);
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.getattr__Ljava_nio_ByteBuffer_Lfuse_FuseGetattrSetter_, jPath, jGetattr);
      if (exception_check_jerrno(env, &jerrno)) break;

      /* public class FuseGetattr extends FuseFtype {

         public long inode;
         // public int mode; in superclass
         public int nlink;
         public int uid;
         public int gid;
         public int rdev;
         public long size;
         public long blocks;
         public int atime;
         public int mtime;
         public int ctime;

         ...}
      */

      // inode support fix by Edwin Olson <eolson@mit.edu>
      stbuf->st_ino =    (ino_t)((*env)->GetLongField(env, jGetattr, FuseGetattr->field.inode));
      stbuf->st_mode =   (mode_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.mode));
      stbuf->st_nlink =  (nlink_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.nlink));
      stbuf->st_uid =    (uid_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.uid));
      stbuf->st_gid =    (gid_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.gid));
      stbuf->st_rdev =   (dev_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.rdev));
      stbuf->st_size =   (off_t)((*env)->GetLongField(env, jGetattr, FuseGetattr->field.size));
      stbuf->st_blocks = (blkcnt_t)((*env)->GetLongField(env, jGetattr, FuseGetattr->field.blocks));
      stbuf->st_atime =  (time_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.atime));
      stbuf->st_mtime =  (time_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.mtime));
      stbuf->st_ctime =  (time_t)((*env)->GetIntField(env, jGetattr, FuseGetattr->field.ctime));

      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);
   if (jGetattr != NULL) (*env)->DeleteLocalRef(env, jGetattr);

   release_env(env);

   return -jerrno;
}


static int javafs_readlink(const char *path, char *buf, size_t size)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jLink = NULL;
   jint jLinkPosition;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jLink = (*env)->NewDirectByteBuffer(env, buf, (jlong)(size - 1));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.readlink__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jPath, jLink);
      if (exception_check_jerrno(env, &jerrno)) break;

      // write a cstring terminator at the end of writen data
      jLinkPosition = (*env)->CallIntMethod(env, jLink, ByteBuffer->method.position);
      if (exception_check_jerrno(env, &jerrno)) break;
      buf[jLinkPosition] = '\0';

      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);
   if (jLink != NULL) (*env)->DeleteLocalRef(env, jLink);

   release_env(env);

   return -jerrno;
}


static int javafs_getdir(const char *path, fuse_dirh_t h, fuse_dirfil_t filler)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobjectArray jDirEntList = NULL;
   jobject jDirEnt = NULL;
   jbyteArray jName = NULL;
   jbyte *jNameBytes = NULL;
   jsize jNameLength;
   jint mode;
   jlong inode;
   jint jerrno = 0;
   jint i;
   jint n;
   int res1;

    TRACE("Start: javafs_getdir");

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jDirEntList = (*env)->NewObject(env, FuseFSDirFiller->class, FuseFSDirFiller->constructor.new);
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.getdir__Ljava_nio_ByteBuffer_Lfuse_FuseFSDirFiller_, jPath, jDirEntList);
      if (exception_check_jerrno(env, &jerrno)) break;

      n = (*env)->CallIntMethod(env, jDirEntList, FuseFSDirFiller->method.size);
      if (exception_check_jerrno(env, &jerrno)) break;

      for (i = 0; i < n; i++)
      {
         char name[MAX_GETDIR_NAME_LENGTH];
         int nameLength = MAX_GETDIR_NAME_LENGTH - 1;

         jDirEnt = (*env)->CallObjectMethod(env, jDirEntList, FuseFSDirFiller->method.get__I, i);
         if (exception_check_jerrno(env, &jerrno)) break;

         mode = (*env)->GetIntField(env, jDirEnt, FuseFSDirEnt->field.mode);
         if (exception_check_jerrno(env, &jerrno)) break;

         jName = (*env)->GetObjectField(env, jDirEnt, FuseFSDirEnt->field.name);
         if (exception_check_jerrno(env, &jerrno)) break;

         inode = (*env)->GetLongField(env, jDirEnt, FuseFSDirEnt->field.inode);
         if (exception_check_jerrno(env, &jerrno)) break;

         jNameBytes = (*env)->GetByteArrayElements(env, jName, NULL);
         if (exception_check_jerrno(env, &jerrno)) break;

         jNameLength = (*env)->GetArrayLength(env, jName);

         if (nameLength > (int)jNameLength)
            nameLength = (int)jNameLength;

         memcpy(name, jNameBytes, nameLength);
         name[nameLength] = '\0';

         res1 = filler(h, name, IFTODT(mode), inode);
         if (res1 != 0)
         {
            jerrno = (jint) res1;
            break;
         }

         (*env)->ReleaseByteArrayElements(env, jName, jNameBytes, JNI_ABORT); jNameBytes = NULL;
         (*env)->DeleteLocalRef(env, jName); jName = NULL;
         (*env)->DeleteLocalRef(env, jDirEnt); jDirEnt = NULL;
      }

      break;
   }

   // cleanup

   if (jNameBytes != NULL) (*env)->ReleaseByteArrayElements(env, jName, jNameBytes, JNI_ABORT);
   if (jName != NULL) (*env)->DeleteLocalRef(env, jName);
   if (jDirEnt != NULL) (*env)->DeleteLocalRef(env, jDirEnt);
   if (jDirEntList != NULL) (*env)->DeleteLocalRef(env, jDirEntList);
   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_mknod(const char *path, mode_t mode, dev_t rdev)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.mknod__Ljava_nio_ByteBuffer_II, jPath, (jint)mode, (jint)rdev);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_mkdir(const char *path, mode_t mode)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.mkdir__Ljava_nio_ByteBuffer_I, jPath, (jint)mode);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_unlink(const char *path)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.unlink__Ljava_nio_ByteBuffer_, jPath);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_rmdir(const char *path)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.rmdir__Ljava_nio_ByteBuffer_, jPath);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_symlink(const char *from, const char *to)
{
   JNIEnv *env = get_env();
   jobject jFrom = NULL;
   jobject jTo = NULL;
   jint jerrno = 0;

   while (1)
   {
      jFrom = (*env)->NewDirectByteBuffer(env, (void *)from, (jlong)strlen(from));
      if (exception_check_jerrno(env, &jerrno)) break;

      jTo = (*env)->NewDirectByteBuffer(env, (void *)to, (jlong)strlen(to));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.symlink__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jFrom, jTo);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jTo != NULL) (*env)->DeleteLocalRef(env, jTo);
   if (jFrom != NULL) (*env)->DeleteLocalRef(env, jFrom);

   release_env(env);

   return -jerrno;
}


static int javafs_rename(const char *from, const char *to)
{
   JNIEnv *env = get_env();
   jobject jFrom = NULL;
   jobject jTo = NULL;
   jint jerrno = 0;

   while (1)
   {
      jFrom = (*env)->NewDirectByteBuffer(env, (void *)from, (jlong)strlen(from));
      if (exception_check_jerrno(env, &jerrno)) break;

      jTo = (*env)->NewDirectByteBuffer(env, (void *)to, (jlong)strlen(to));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.rename__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jFrom, jTo);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jTo != NULL) (*env)->DeleteLocalRef(env, jTo);
   if (jFrom != NULL) (*env)->DeleteLocalRef(env, jFrom);

   release_env(env);

   return -jerrno;
}


static int javafs_link(const char *from, const char *to)
{
   JNIEnv *env = get_env();
   jobject jFrom = NULL;
   jobject jTo = NULL;
   jint jerrno = 0;

   while (1)
   {
      jFrom = (*env)->NewDirectByteBuffer(env, (void *)from, (jlong)strlen(from));
      if (exception_check_jerrno(env, &jerrno)) break;

      jTo = (*env)->NewDirectByteBuffer(env, (void *)to, (jlong)strlen(to));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.link__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jFrom, jTo);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jTo != NULL) (*env)->DeleteLocalRef(env, jTo);
   if (jFrom != NULL) (*env)->DeleteLocalRef(env, jFrom);

   release_env(env);

   return -jerrno;
}


static int javafs_chmod(const char *path, mode_t mode)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.chmod__Ljava_nio_ByteBuffer_I, jPath, (jint)mode);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_chown(const char *path, uid_t uid, gid_t gid)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.chown__Ljava_nio_ByteBuffer_II, jPath, (jint)uid, (jint)gid);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_truncate(const char *path, off_t size)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.truncate__Ljava_nio_ByteBuffer_J, jPath, (jlong)size);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_utime(const char *path, struct utimbuf *buf)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      (*env)->CallVoidMethod(env, fuseFS, FuseFS->method.utime__Ljava_nio_ByteBuffer_II, jPath, (jint)(buf->actime), (jint)(buf->modtime));
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_open(const char *path, struct fuse_file_info *ffi)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jOpen = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jOpen = (*env)->NewObject(env, FuseOpen->class, FuseOpen->constructor.new);
      if (exception_check_jerrno(env, &jerrno)) break;

      (*env)->SetBooleanField(env, jOpen, FuseOpen->field.directIO, ffi->direct_io ? JNI_TRUE : JNI_FALSE);
      (*env)->SetBooleanField(env, jOpen, FuseOpen->field.keepCache, ffi->keep_cache ? JNI_TRUE : JNI_FALSE);

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.open__Ljava_nio_ByteBuffer_ILfuse_FuseOpenSetter_, jPath, (jint)(ffi->flags), jOpen);
      if (exception_check_jerrno(env, &jerrno)) break;

      // if fh is non null then create a global reference to it (will be released in release callback)
      jobject jFh = (*env)->GetObjectField(env, jOpen, FuseOpen->field.fh);
      jobject jFhGlobalRef = (jFh == NULL) ? NULL : (*env)->NewGlobalRef(env, jFh);

      // every sane platform should store a pointer into unsigned long without a problem
      create_file_handle(ffi, jFhGlobalRef);
      ffi->direct_io = ((*env)->GetBooleanField(env, jOpen, FuseOpen->field.directIO) == JNI_TRUE)? 1 : 0;
      ffi->keep_cache = ((*env)->GetBooleanField(env, jOpen, FuseOpen->field.keepCache) == JNI_TRUE)? 1 : 0;

      // remove local reference to fh
      if (jFh != NULL) (*env)->DeleteLocalRef(env, jFh);

      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);
   if (jOpen != NULL) (*env)->DeleteLocalRef(env, jOpen);

   release_env(env);

   return -jerrno;
}


static int javafs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *ffi)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jBuf = NULL;
   jint jerrno = 0;
   jint nread = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jBuf = (*env)->NewDirectByteBuffer(env, buf, (jlong)size);
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.read__Ljava_nio_ByteBuffer_Ljava_lang_Object_Ljava_nio_ByteBuffer_J, jPath, read_file_handle(ffi), jBuf, (jlong)offset);
      if (exception_check_jerrno(env, &jerrno)) break;

      // to obtain # of bytes read, get current position from ByteBuffer
      nread = (*env)->CallIntMethod(env, jBuf, ByteBuffer->method.position);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jBuf != NULL) (*env)->DeleteLocalRef(env, jBuf);
   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return jerrno? -jerrno : nread;
}


static int javafs_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *ffi)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jBuf = NULL;
   jint jerrno = 0;
   jint nwriten = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jBuf = (*env)->NewDirectByteBuffer(env, (void *)buf, (jlong)size);
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.write__Ljava_nio_ByteBuffer_Ljava_lang_Object_ZLjava_nio_ByteBuffer_J, jPath, read_file_handle(ffi), (ffi->writepage)? JNI_TRUE : JNI_FALSE, jBuf, (jlong)offset);
      if (exception_check_jerrno(env, &jerrno)) break;

      // to obtain # of bytes writen, get current position from ByteBuffer
      nwriten = (*env)->CallIntMethod(env, jBuf, ByteBuffer->method.position);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jBuf != NULL) (*env)->DeleteLocalRef(env, jBuf);
   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return jerrno? -jerrno : nwriten;
}


static int javafs_statvfs(const char *path, struct statvfs *fst)
{
   JNIEnv *env = get_env();
   jobject jStatfs = NULL;
   jint jerrno = 0;


   TRACE("Start: javafs_statvfs");

   while (1)
   {
      TRACE("HERE 1");
      jStatfs = (*env)->NewObject(env, FuseStatfs->class, FuseStatfs->constructor.new);
      if (exception_check_jerrno(env, &jerrno)) break;
      TRACE("HERE 2 %p, %p", env, fuseFS);

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.statfs__Lfuse_FuseStatfsSetter_, jStatfs);

      TRACE("HERE 2A");

      if (exception_check_jerrno(env, &jerrno)) break;

      TRACE("HERE 3");

      fst->f_bsize   = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.blockSize);
      TRACE("HERE 4");
      fst->f_blocks  = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.blocks);
      TRACE("HERE 5");
      fst->f_bfree   = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.blocksFree);
      TRACE("HERE 6");
      fst->f_bavail  = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.blocksAvail);
      TRACE("HERE 7");
      fst->f_files   = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.files);
      TRACE("HERE 8");
      fst->f_ffree   = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.filesFree);
      TRACE("HERE 9");
      fst->f_namemax = (long) (*env)->GetIntField(env, jStatfs, FuseStatfs->field.namelen);
      TRACE("HERE 10");
      break;
   }

   // cleanup

   if (jStatfs != NULL) (*env)->DeleteLocalRef(env, jStatfs);
   TRACE("HERE 11");

   release_env(env);

   return -jerrno;
}


static int javafs_flush(const char *path, struct fuse_file_info *ffi)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.flush__Ljava_nio_ByteBuffer_Ljava_lang_Object_, jPath, read_file_handle(ffi));
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


static int javafs_release(const char *path, struct fuse_file_info *ffi)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jFh = read_file_handle(ffi);
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.release__Ljava_nio_ByteBuffer_Ljava_lang_Object_I, jPath, jFh, (jint)(ffi->flags));
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   // jFh is global reference and should be released in release callback
   if (jFh != NULL)
   {
      (*env)->DeleteGlobalRef(env, jFh);
      (*env)->DeleteLocalRef(env, jFh);
      ffi->fh = 0;
   }

   release_env(env);

   return -jerrno;
}


static int javafs_fsync(const char *path, int datasync, struct fuse_file_info *ffi)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.fsync__Ljava_nio_ByteBuffer_Ljava_lang_Object_Z, jPath, read_file_handle(ffi), (jint)(ffi->flags), datasync? JNI_TRUE : JNI_FALSE);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return -jerrno;
}


//
// extended attributes support contributed by Steven Pearson <steven_pearson@final-step.com>
// and then modified by Peter Levart <peter@select-tech.si> to fit the new errno returning scheme


/*
static int javafs_setxattr(const char *path, const char *name, const char *value, size_t size, int flags)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jName = NULL;
   jobject jValue = NULL;
   jint jerrno = 0;

   while(1)
   {
      jobject jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jobject jName = (*env)->NewDirectByteBuffer(env, (void *)name, (jlong)strlen(name));
      if (exception_check_jerrno(env, &jerrno)) break;

      jobject jValue = (*env)->NewDirectByteBuffer(env, (void *)value, (jlong)size);
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.setxattr__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_I, jPath, jName, jValue, (jint)flags);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);
   if (jName != NULL) (*env)->DeleteLocalRef(env, jName);
   if (jValue != NULL) (*env)->DeleteLocalRef(env, jValue);

   release_env(env);

   return -jerrno;
}
*/

/*
static int javafs_getxattr(const char *path, const char *name, char *value, size_t size)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jName = NULL;
   jobject jValue = NULL;
   jobject jSize = NULL;
   jint jerrno = 0;
   jint xattrsize;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jName = (*env)->NewDirectByteBuffer(env, (void *)name, (jlong)strlen(name));
      if (exception_check_jerrno(env, &jerrno)) break;

      // Size of the attribute
      if (size == 0)
      {
         jSize = (*env)->NewObject(env, FuseSize->class, FuseSize->constructor.new);
         if (exception_check_jerrno(env, &jerrno)) break;

         jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.getxattrsize__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_Lfuse_FuseSizeSetter_, jPath, jName, jSize);
         if (exception_check_jerrno(env, &jerrno)) break;

         xattrsize = (*env)->GetIntField(env, jSize, FuseSize->field.size);
      }
      else
      {
         jValue = (*env)->NewDirectByteBuffer(env, value , (jlong)size);
         if (exception_check_jerrno(env, &jerrno)) break;

         jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.getxattr__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jPath, jName, jValue);
         if (exception_check_jerrno(env, &jerrno)) break;

         // to obtain # of bytes read, get current position from ByteBuffer
         xattrsize = (*env)->CallIntMethod(env, jValue, ByteBuffer->method.position);
      }

      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jSize != NULL) (*env)->DeleteLocalRef(env, jSize);
   if (jValue != NULL) (*env)->DeleteLocalRef(env, jValue);
   if (jName != NULL) (*env)->DeleteLocalRef(env, jName);
   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);

   release_env(env);

   return jerrno? -jerrno : xattrsize;
}
*/

static int javafs_listxattr(const char *path, char *list, size_t size)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jList = NULL;
   jobject jSize = NULL;
   jint jerrno = 0;
   jint xattrsize;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      // Size of the attribute list
      if (size == 0)
      {
         jSize = (*env)->NewObject(env, FuseSize->class, FuseSize->constructor.new);
         if (exception_check_jerrno(env, &jerrno)) break;

         jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.listxattrsize__Ljava_nio_ByteBuffer_Lfuse_FuseSizeSetter_, jPath, jSize);
         if (exception_check_jerrno(env, &jerrno)) break;

         xattrsize = (*env)->GetIntField(env, jSize, FuseSize->field.size);
      }
      else
      {
         jList = (*env)->NewDirectByteBuffer(env, list , (jlong)size);
         if (exception_check_jerrno(env, &jerrno)) break;

         jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.listxattr__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jPath, jList);
         if (exception_check_jerrno(env, &jerrno)) break;

         // to obtain # of bytes read, get current position from ByteBuffer
         xattrsize = (*env)->CallIntMethod(env, jList, ByteBuffer->method.position);
      }

      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jSize != NULL) (*env)->DeleteLocalRef(env, jSize);
   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);
   if (jList != NULL) (*env)->DeleteLocalRef(env, jList);

   release_env(env);

   return jerrno? -jerrno : xattrsize;
}

static int javafs_removexattr(const char *path, const char *name)
{
   JNIEnv *env = get_env();
   jobject jPath = NULL;
   jobject jName = NULL;
   jint jerrno = 0;

   while (1)
   {
      jPath = (*env)->NewDirectByteBuffer(env, (void *)path, (jlong)strlen(path));
      if (exception_check_jerrno(env, &jerrno)) break;

      jName = (*env)->NewDirectByteBuffer(env, (void *)name, (jlong)strlen(name));
      if (exception_check_jerrno(env, &jerrno)) break;

      jerrno = (*env)->CallIntMethod(env, fuseFS, FuseFS->method.removexattr__Ljava_nio_ByteBuffer_Ljava_nio_ByteBuffer_, jPath, jName);
      exception_check_jerrno(env, &jerrno);
      break;
   }

   // cleanup

   if (jPath != NULL) (*env)->DeleteLocalRef(env, jPath);
   if (jName != NULL) (*env)->DeleteLocalRef(env, jName);

   release_env(env);

   return -jerrno;
}

/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
static void * javafs_init(struct fuse_conn_info *conn) {
    jfuse_params *params = fuse_get_context()->private_data;
    JNIEnv *env;

    if ((env = alloc_JVM(params->javaArgc, params->javaArgv)) != NULL)
    {
        if (alloc_classes(env))
        {
            if (alloc_filesystem(env, params->filesystemClassName))
            {
            }
        }
    }
    return params;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
static void javafs_destroy(void *data) {

    TRACE("Destroy Starting");
    JNIEnv *env = get_env();

    free_fuseFS(env);
    free_classes(env);

    if ((*env)->ExceptionCheck(env))
         (*env)->ExceptionClear(env);

    free_JVM(env);
    TRACE("Destroy Complete");
}


struct fuse_operations javafs_oper = {
   getattr:    javafs_getattr,
   readlink:   javafs_readlink,
   getdir:     javafs_getdir,
   mknod:      javafs_mknod,
   mkdir:      javafs_mkdir,
   symlink:    javafs_symlink,
   unlink:     javafs_unlink,
   rmdir:      javafs_rmdir,
   rename:     javafs_rename,
   link:       javafs_link,
   chmod:      javafs_chmod,
   chown:      javafs_chown,
   truncate:   javafs_truncate,
   utime:      javafs_utime,
   open:       javafs_open,
   read:       javafs_read,
   write:      javafs_write,
   statfs:     javafs_statvfs,
   flush:      javafs_flush,
   release:    javafs_release,
   fsync:      javafs_fsync,
   // extended attributes are now implemented
   setxattr:    NULL,
   getxattr:    NULL,
   listxattr:   javafs_listxattr,
   removexattr: javafs_removexattr,
   opendir:     NULL,
   readdir:     NULL,
   releasedir:  NULL,
   fsyncdir:    NULL,
   init:        javafs_init,
   destroy:     javafs_destroy,
   access:      NULL,
   create:      NULL,
   ftruncate:   NULL,
   fgetattr:    NULL
};
