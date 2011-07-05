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

import fuse.FuseStatfs;
import fuse.compat.FuseDirEnt;
import fuse.compat.FuseStat;

import java.nio.ByteBuffer;

/**
 * interface HdfsClient
 *
 */
interface HdfsClient {

  public FuseStatfs getStatus();

  public FuseStat getFileInfo(String path);

  public FuseDirEnt[] listPaths(String path);

  /**
   * @param path
   * @return Object --> hdfsFile, that should be passed to close()
   */
  public Object openForRead(String path);

  public Object createForWrite(String path);

  public boolean close(Object hdfsFile);

  public boolean read(Object hdfsFile, ByteBuffer buf, long offset);

  public boolean write(Object hdfsFile, ByteBuffer buf, long offset);

  public boolean mkdir(String path);

  public boolean unlink(String filePath);

  public boolean rmdir(String dirPath);

  public boolean rename(String src, String dst);
}