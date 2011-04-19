#!/bin/sh

#
# Usage: ./hadoopfs_fuse_mount.sh [options] /mount/point
#
# The Hadoop configuration settings (i.e. hadoop-site.xml) is picked up
# via the classpath below: (e.g. from ${HADOOP_HOME}/conf)
#

VERSION=2.4.0.0-SNAPSHOT
HADOOP_HOME=/data/projects/hadoop-common/
FUSE4J_NATIVE_DIR=../native
FUSE4J_HDFS_DIR=../maven/fuse4j-hdfs/target/fuse4j-hdfs-${VERSION}-bin/fuse4j-hdfs-${VERSION}/


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
# get all the hdfs jars to load
#
for file in `find $FUSE4J_HDFS_DIR/ | grep ".jar"`
do
    FUSE4J_HDFS_JARS=$FUSE4J_HDFS_JARS:$file
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
LD_LIBRARY_PATH=$FUSE4J_NATIVE_DIR $PATH_TO_JAVA \
   -classpath $FUSE4J_HDFS_JARS:$HADOOP_CONF_DIR:$HADOOP_JARS:$FUSE4J_HDFS_JARS \
   -Dorg.apache.commons.logging.Log=fuse.logging.FuseLog \
   -Dfuse.logging.level=DEBUG \
   com.ibm.fusejhadoopfs.FuseHdfsClient -f -s $@
