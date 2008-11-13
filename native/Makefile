
JAVA_HOME := /System/Library/Frameworks/JavaVM.framework/Versions/1.5/Home
SRCS := javafs.c javafs_bindings.c fuse_callback.c
HDRS := javafs.h javafs_bindings.h
FS_EXE := javafs

INCLUDES := -I${JAVA_HOME}/include -I${FUSE_HOME}/include

LDPATH := ${LDPATH} -L${FUSE_HOME}/lib /System/Library/Frameworks/JavaVM.framework/Versions/1.5/Libraries


all: ${FS_EXE}

${FS_EXE}: ${SRCS} ${HDRS}
	gcc -Wall -c -fPIC -D__FreeBSD__=10 -D_FILE_OFFSET_BITS=64 ${INCLUDES} javafs.c -o javafs.o
	gcc -Wall -c -fPIC -D__FreeBSD__=10 -D_FILE_OFFSET_BITS=64 ${INCLUDES} fuse_callback.c -o fuse_callback.o
	gcc -Wall -c -fPIC -D__FreeBSD__=10 -D_FILE_OFFSET_BITS=64 ${INCLUDES} javafs_bindings.c -o javafs_bindings.o
	gcc -Wall -fPIC -D__FreeBSD__=10 -D_FILE_OFFSET_BITS=64 -framework CoreFoundation -framework IOKit -framework JavaVM -lfuse -lpthread -lSystem -o ${FS_EXE} javafs.o fuse_callback.o javafs_bindings.o


clean:
	rm -f ${FS_EXE} *.o

