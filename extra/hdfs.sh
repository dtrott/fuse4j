#!/bin/bash

#
# Usage: ./mount_hdfs.sh [options] /mount/point
#
# The Hadoop configuration settings (i.e. hadoop-site.xml) is picked up
# via the classpath below: (e.g. from ${HADOOP_HOME}/conf)
#

this="$0"
while [ -h "$this" ]; do
  ls=`ls -ld "$this"`
  link=`expr "$ls" : '.*-> \(.*\)$'`
  if expr "$link" : '.*/.*' > /dev/null; then
    this="$link"
  else
    this=`dirname "$this"`/"$link"
  fi
done
# convert relative path to absolute path
SCRIPT_DIR=`dirname "$this"`
SCRIPT_DIR=`cd "$SCRIPT_DIR" && pwd`
#chdir to this script dir
cd $SCRIPT_DIR

VERSION=2.4.0.0-SNAPSHOT
HADOOP_HOME=/home/hadoop/hadoop
FUSE4J_NATIVE_DIR=$SCRIPT_DIR/../native
FUSE4J_HDFS_JAR=$SCRIPT_DIR/../maven/fuse4j-hadoopfs/fuse4j-hadoopfs-${VERSION}.jar
FUSE4J_CORE_JAR=$SCRIPT_DIR/../maven/fuse4j-core/fuse4j-core-${VERSION}.jar

#
# figure out which java to use
#
PATH_TO_JAVA=java

if [ "$JAVA_HOME" != "" ]; then
    PATH_TO_JAVA=$JAVA_HOME/bin/java
fi

#
# get all the hadoop jars to load
#
for file in `find $HADOOP_HOME/ | grep ".jar"`
do
    HADOOP_JARS=$HADOOP_JARS:$file
done

#
# if a conf directory is not defined, then point to the default location
#

if [ "$HADOOP_CONF_DIR" = "" ]; then
    HADOOP_CONF_DIR=$HADOOP_HOME/conf
fi

#
# startup fuse-j-hdfs
#
export LD_LIBRARY_PATH=$FUSE4J_NATIVE_DIR
export CLASSPATH=$SCRIPT_DIR:$FUSE4J_HDFS_JAR:$FUSE4J_CORE_JAR:$HADOOP_CONF_DIR:$HADOOP_JARS
PARAMS=$@
echo $PARAMS
#nohup env LD_LIBRARY_PATH=$FUSE4J_NATIVE_DIR CLASSPATH=$CLASSPATH $PATH_TO_JAVA -Xmx128m  -Xms16m -Dcom.sun.management.jmxremote.port=39090 -Dcom.sun.management.jmxremote.authenticate=false -Dorg.apache.commons.logging.Log=fuse.logging.FuseLog -Dfuse.logging.level=DEBUG fuse4j.hadoopfs.FuseHdfsClient -f $PARAMS &>/dev/null &
nohup env LD_LIBRARY_PATH=$FUSE4J_NATIVE_DIR CLASSPATH=$CLASSPATH $PATH_TO_JAVA -Xmx128m  -Xms16m -Dorg.apache.commons.logging.Log=fuse.logging.FuseLog -Dfuse.logging.level=INFO fuse4j.hadoopfs.FuseHdfsClient -f $PARAMS &>/dev/null &

