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

FUSEMOUNT_CMD="$SCRIPT_DIR/hdfs.sh $@"

su hadoop -c "`echo $FUSEMOUNT_CMD`"

