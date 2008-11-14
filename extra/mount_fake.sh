#!/bin/sh

PROJECT_NAME=fuse4j
CWD=`pwd`

FUSE_HOME=/usr/local
MOUNT_POINT=${CWD}/fake
FS_CLASS=fuse/FakeFilesystem
VERSION=2.4.0.0-SNAPSHOT


M2_REPO=${HOME}/.m2/repository

LD_LIBRARY_PATH=$FUSE_HOME/lib
export LD_LIBRARY_PATH

CLASSPATH=""
CLASSPATH="$CLASSPATH:$M2_REPO/${PROJECT_NAME}/${PROJECT_NAME}-core/${VERSION}/${PROJECT_NAME}-core-${VERSION}.jar"
CLASSPATH="$CLASSPATH:$M2_REPO/${PROJECT_NAME}/${PROJECT_NAME}-fakefs/${VERSION}/${PROJECT_NAME}-fakefs-${VERSION}.jar"
CLASSPATH="$CLASSPATH:$M2_REPO/commons-logging/commons-logging/1.1.1/commons-logging-1.1.1.jar:$M2_REPO/log4j/log4j/1.2.13/log4j-1.2.13.jar:${CWD}"
export CLASSPATH


../native/javafs -C${FS_CLASS} "-J-Djava.class.path=$CLASSPATH" $MOUNT_POINT -f
