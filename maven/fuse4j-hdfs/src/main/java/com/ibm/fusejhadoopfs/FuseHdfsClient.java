/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.ibm.fusejhadoopfs;

import fuse.*;
import fuse.compat.Filesystem1;
import fuse.compat.FuseDirEnt;
import fuse.compat.FuseStat;

import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.Set;
import java.util.Iterator;

import fuse.logging.FuseLog;

/**
 * class FuseHdfsClient
 *
 */
class FuseHdfsClient implements Filesystem1, Runnable {
  //
  // Members
  //

  private final FuseLog log = new FuseLog("FuseHdfsClient.Log");

  private HdfsClient hdfs = null;

  private HashMap<String, HdfsFileContext> hdfsFileCtxtMap = null;

  private Thread ctxtMapCleanerThread = null;

  /**
   * ctor
   */
  FuseHdfsClient() {
    hdfs = HdfsClientFactory.create(HdfsClientFactory.HdfsClientReal);

    hdfsFileCtxtMap = new HashMap<String, HdfsFileContext>();

    ctxtMapCleanerThread = new Thread(this);
    ctxtMapCleanerThread.start();
  }

  //
  // Filesystem1 interface API's
  //

  /**
   * getattr()
   */
  public FuseStat getattr(String path) throws FuseException {
    log.info("getattr(): " + path + "\n");

    FuseStat s = hdfs.getFileInfo(path);

    if (s == null) {
      throw new FuseException("no such entry").initErrno(FuseException.ENOENT);
    }

    return s;
  }

  /**
   * readlink()
   */
  public String readlink(String path) throws FuseException {
    log.info("readlink(): " + path + "\n");
    throw new FuseException("readlink not supported")
        .initErrno(FuseException.ENOSYS);
  }

  /**
   * getdir()
   */
  public FuseDirEnt[] getdir(String path) throws FuseException {
    log.info("getdir(): " + path + "\n");

    FuseDirEnt[] entries = hdfs.listPaths(path);

    if (entries == null) {
      throw new FuseException("no entries").initErrno(FuseException.ENOTDIR);
    }

    return entries;
  }

  /**
   * mknod()
   */
  public void mknod(String path, int mode, int rdev) throws FuseException {
    log.info("mknod(): " + path + " mode: " + mode + "\n");

    // do a quick check to see if the file already exists
    HdfsFileContext ctxt = pinFileContext(path);

    if (ctxt != null) {
      unpinFileContext(path);
      throw new FuseException("file exists").initErrno(FuseException.EPERM);
    }

    // create the file
    Object hdfsFile = hdfs.createForWrite(path);

    //
    // track this newly opened file, for writing.
    //

    //
    // there will be no 'release' for this mknod, therefore we must not
    // pin the file, it will still live in the tree, but will have a 
    // '0' pin-count.
    // TODO: eventually have a worker-thread that looks at all
    //       '0' pin-count objects, ensures that they were opened for
    //       write and releases() them if no writes have happened.
    //       (this hack is to support HDFS's write-once semantics).
    //

    if (!addFileContext(path, new HdfsFileContext(hdfsFile, true), false)) {
      // if we raced with another thread, then close off the open file
      // and fail this open().
      hdfs.close(hdfsFile);

      // TODO: don't fail this open() when racing with another
      //       thread, instead just use the 
      //       already inserted 'context'...?
      throw new FuseException("collision").initErrno(FuseException.EACCES);
    }
  }

  public void mkdir(String path, int mode) throws FuseException {
    log.info("mkdir(): " + path + " mode: " + mode + "\n");

    boolean status = hdfs.mkdir(path);

    if (!status) {
      throw new FuseException("dir not created")
          .initErrno(FuseException.EACCES);
    }
  }

  public void unlink(String path) throws FuseException {
    log.info("unlink(): " + path + "\n");

    boolean status = hdfs.unlink(path);

    if (!status) {
      throw new FuseException("file not deleted")
          .initErrno(FuseException.EACCES);
    }
  }

  public void rmdir(String path) throws FuseException {
    log.info("rmdir(): " + path + "\n");

    boolean status = hdfs.rmdir(path);

    if (!status) {
      throw new FuseException("dir not deleted")
          .initErrno(FuseException.EACCES);
    }

  }

  public void symlink(String from, String to) throws FuseException {
    log.info("symlink(): " + from + "\n");
    throw new FuseException("symlink not supported")
        .initErrno(FuseException.ENOSYS);
  }

  public void rename(String from, String to) throws FuseException {
    log.info("rename(): " + from + " to: " + to + "\n");

    boolean status = hdfs.rename(from, to);

    if (!status) {
      throw new FuseException("file not renamed")
          .initErrno(FuseException.EPERM);
    }

  }

  public void link(String from, String to) throws FuseException {
    log.info("link(): " + from + "\n");
    throw new FuseException("link not supported")
        .initErrno(FuseException.ENOSYS);
  }

  public void chmod(String path, int mode) throws FuseException {
    log.info("chmod(): " + path + "\n");
    throw new FuseException("chmod not supported")
        .initErrno(FuseException.ENOSYS);
  }

  public void chown(String path, int uid, int gid) throws FuseException {
    log.info("chown(): " + path + "\n");
    throw new FuseException("chown not supported")
        .initErrno(FuseException.ENOSYS);
  }

  public void truncate(String path, long size) throws FuseException {
    log.info("truncate(): " + path + " size: " + size + "\n");
    throw new FuseException("truncate not supported")
        .initErrno(FuseException.EPERM);
  }

  public void utime(String path, int atime, int mtime) throws FuseException {
    // not supported right now (write-once files...)
    log.info("utime(): " + path + "\n");
    throw new FuseException("utime not supported")
        .initErrno(FuseException.ENOSYS);
  }

  public FuseStatfs statfs() throws FuseException {
    // TODO: support this in the future?
    log.info("statfs(): \n");
    throw new FuseException("statfs not supported")
        .initErrno(FuseException.ENOSYS);
  }

  /**
   * open()
   */
  public void open(String path, int flags) throws FuseException {
    log.info("open(): " + path + " flags " + flags + "\n");

    HdfsFileContext ctxt = pinFileContext(path);

    //
    // if this context is alive due to a recent mknod() operation
    // then we will allow subsequent open()'s for write.
    // until the first release().
    //
    if (ctxt != null) {
      // from here on, if we fail, we will have to unpin the file-context
      if (ctxt.openedForWrite) {
        // return true only for write access
        if (isWriteOnlyAccess(flags)) {
          return;
        }

        // we cannot support read/write at this time, since the
        // file has only been opened for 'writing' on HDFS
        unpinFileContext(path);
        throw new FuseException("only writes allowed")
            .initErrno(FuseException.EACCES);
      }

      // if context has been opened already for reading,
      // then return true only for read access
      if (isReadOnlyAccess(flags)) {
        return;
      }

      // we cannot support write at this time, since the
      // file has only been opened for 'writing' on HDFS
      unpinFileContext(path);
      throw new FuseException("only reads allowed")
          .initErrno(FuseException.EACCES);
    }

    //
    // if we are here, then the ctxt must be null, and we will open
    // the file for reading via HDFS.
    //

    // we will only support open() for 'read'
    if (!isReadOnlyAccess(flags)) {
      throw new FuseException("only reads allowed")
          .initErrno(FuseException.EACCES);
    }

    // open the file
    Object hdfsFile = hdfs.openForRead(path);

    if (hdfsFile == null) {
      throw new FuseException("uknown error in open()")
          .initErrno(FuseException.EACCES);
    }

    // track this newly opened file, for reading.
    if (!addFileContext(path, new HdfsFileContext(hdfsFile, false), true)) {
      // if we raced with another thread, then close off the open file
      // and fail this open().
      hdfs.close(hdfsFile);

      // TODO: don't fail this open() when racing with another
      //       thread, instead just use the 
      //       already inserted 'context'...?
      throw new FuseException("collision").initErrno(FuseException.EACCES);
    }
  }

  /**
   * read()
   */
  public void read(String path, ByteBuffer buf, long offset)
      throws FuseException {
    log.info("read(): " + path + " offset: " + offset + " len: "
        + buf.capacity() + "\n");

    HdfsFileContext ctxt = pinFileContext(path);

    if (ctxt == null) {
      throw new FuseException("file not opened").initErrno(FuseException.EPERM);
    }

    if (ctxt.openedForWrite) {
      unpinFileContext(path);
      throw new FuseException("file not opened for read")
          .initErrno(FuseException.EPERM);
    }

    boolean status = hdfs.read(ctxt.hdfsFile, buf, offset);

    unpinFileContext(path);

    if (!status) {
      throw new FuseException("read failed").initErrno(FuseException.EACCES);
    }
  }

  /**
   * write()
   */
  public void write(String path, ByteBuffer buf, long offset)
      throws FuseException {
    log.info("write(): " + path + " offset: " + offset + " len: "
        + buf.capacity() + "\n");

    HdfsFileContext ctxt = pinFileContext(path);

    if (ctxt == null) {
      throw new FuseException("file not opened").initErrno(FuseException.EPERM);
    }

    if (!ctxt.openedForWrite) {
      unpinFileContext(path);

      throw new FuseException("file not opened for write")
          .initErrno(FuseException.EPERM);
    }

    boolean status = hdfs.write(ctxt.hdfsFile, buf, offset);

    unpinFileContext(path);

    if (!status) {
      throw new FuseException("write failed").initErrno(FuseException.EACCES);
    }
  }

  /**
   * release()
   */
  public void release(String path, int flags) throws FuseException {
    log.info("release(): " + path + " flags: " + flags + "\n");
    unpinFileContext(path);
  }

  //
  // Private Methods
  //

  /**
   * isWriteOnlyAccess()
   */
  private boolean isWriteOnlyAccess(int flags) {
    return ((flags & 0x0FF) == FilesystemConstants.O_WRONLY);
  }

  /**
   * isReadOnlyAccess()
   */
  private boolean isReadOnlyAccess(int flags) {
    return ((flags & 0x0FF) == FilesystemConstants.O_RDONLY);
  }

  /**
   * addFileContext()
   * @return false if the path is already tracked
   */
  private boolean addFileContext(String path, HdfsFileContext ctxt,
      boolean pinFile) {
    boolean addedCtxt = false;

    synchronized (hdfsFileCtxtMap) {
      Object o = hdfsFileCtxtMap.get(path);
      if (o == null) {
        if (pinFile) {
          ctxt.pinCount = 1;
        }
        hdfsFileCtxtMap.put(path, ctxt);
        addedCtxt = true;
      }
    }

    return addedCtxt;
  }

  /**
   * pinFileContext()
   */
  private HdfsFileContext pinFileContext(String path) {
    HdfsFileContext ctxt = null;

    synchronized (hdfsFileCtxtMap) {
      Object o = hdfsFileCtxtMap.get(path);
      if (o != null) {
        ctxt = (HdfsFileContext) o;
        ctxt.pinCount++;
      }
    }

    return ctxt;
  }

  /**
   * unpinFileContext()
   */
  private void unpinFileContext(String path) {
    synchronized (hdfsFileCtxtMap) {
      Object o = hdfsFileCtxtMap.get(path);

      if (o != null) {
        HdfsFileContext ctxt = (HdfsFileContext) o;

        ctxt.pinCount--;

        if (ctxt.pinCount == 0) {
          unprotectedCleanupFileContext(path, ctxt);
          hdfsFileCtxtMap.remove(path);
        }
      }
    }
  }

  /**
   * cleanupExpiredFileContexts()
   */
  private void cleanupExpiredFileContexts() {
    synchronized (hdfsFileCtxtMap) {
      Set paths = hdfsFileCtxtMap.keySet();
      Iterator pathsIter = paths.iterator();

      while (pathsIter.hasNext()) {
        String path = (String) pathsIter.next();

        // remove expired file-contexts
        HdfsFileContext ctxt = hdfsFileCtxtMap.get(path);
        if (ctxt.expired()) {
          // close this context, and remove from the map
          unprotectedCleanupFileContext(path, ctxt);
          pathsIter.remove();
        }
      }
    }
  }

  /**
   * unprotectedCleanupFileContext()
   */
  private void unprotectedCleanupFileContext(String path, HdfsFileContext ctxt) {
    log.info("closing...(): " + path + "\n");

    hdfs.close(ctxt.hdfsFile);
  }

  //
  // HdfsFileContext garbage collection
  //

  /**
   * run()
   * - this method is called on a thread, it sleepily trolls the
   *   HdfsFileContext-map to see if any expired contexts need to
   *   be shutdown (this can happen if a mknod() occurred, but the 
   *   associated open()/write()/release() did not happen in which
   *   case we shutdown the file on HDFS after a set period of time,
   *   and the file becomes read-only.
   */
  public void run() {
    // run once a minute...
    final long timeToSleep = 10000;

    try {
      while (true) {
        // wait for a bit, before checking for expired contexts
        Thread.sleep(timeToSleep);

        cleanupExpiredFileContexts();
      }
    } catch (InterruptedException inte) {
      log.info("run(): " + inte + "\n");
      return;
    }
  }

  //
  // Entry Point
  //
  public static void main(String[] args) {
    try {
      FuseMount.mount(args, new FuseHdfsClient());
    } catch (Exception e) {
      e.printStackTrace();
    }
  }
}

/**
 * class HdfsFileContext
 *
 */
class HdfsFileContext {
  public Object hdfsFile = null;

  public boolean openedForWrite = false;

  public long pinCount = 0;

  private long creationTime = 0;

  // longest time this context can live with a pinCount of 0
  //  - 20 seconds (in ms)
  private static final long expiryTime = 20000;

  /**
   * @param hdfsFile
   * @param openedForWrite
   */
  public HdfsFileContext(Object hdfsFile, boolean openedForWrite) {
    this.hdfsFile = hdfsFile;
    this.openedForWrite = openedForWrite;
    this.creationTime = System.currentTimeMillis();
  }

  public boolean expired() {
    return ((pinCount == 0) && ((System.currentTimeMillis() - creationTime) > expiryTime));
  }
}