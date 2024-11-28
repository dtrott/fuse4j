#!/bin/sh

#JAVA_HOME=  -- YOU MUST SET JAVA_HOME --

# This script uses the native (executable) launcher which works in either the foreground or forked into the background.

PROJECT_NAME=fuse4j
CWD=`pwd`

FUSE_HOME=/usr/local
MOUNT_POINT=${CWD}/fake
FS_CLASS=fuse/FakeFilesystem
VERSION=2.4.0.0-SNAPSHOT


M2_REPO=${HOME}/.m2/repository

LD_LIBRARY_PATH=$FUSE_HOME/lib:${JAVA_HOME}/lib/server
export LD_LIBRARY_PATH

CLASSPATH=""
CLASSPATH="$CLASSPATH:$M2_REPO/${PROJECT_NAME}/${PROJECT_NAME}-core/${VERSION}/${PROJECT_NAME}-core-${VERSION}.jar"
CLASSPATH="$CLASSPATH:$M2_REPO/${PROJECT_NAME}/${PROJECT_NAME}-fakefs/${VERSION}/${PROJECT_NAME}-fakefs-${VERSION}.jar"
CLASSPATH="$CLASSPATH:$M2_REPO/org/slf4j/slf4j-api/2.0.16/slf4j-api-2.0.16.jar:$M2_REPO/org/slf4j/slf4j-jdk14/2.0.16/slf4j-jdk14-2.0.16.jar:${CWD}"
export CLASSPATH


mkdir -p $MOUNT_POINT
../native/javafs $MOUNT_POINT -f -o class=${FS_CLASS} -o "jvm=-Djava.class.path=$CLASSPATH" 
