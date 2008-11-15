SRCS=javafs.c javafs_bindings.c fuse_callback.c native_impl.c
OBJS=javafs.o javafs_bindings.o fuse_callback.o native_impl.o

include make.flags

DEPEND=makedepend $(DEPFLAGS)

all: ${FS_EXE} ${LIB_SO}

dep:
	${DEPEND} ${INCLUDES} -fmake.depend ${SRCS}

.c.o:
	${CC} -c ${CFLAGS} ${INCLUDES} $*.c

${FS_EXE}: ${OBJS}
	${CC} ${CFLAGS} ${LDFLAGS} ${LDPATH} -o ${FS_EXE} ${OBJS}

${LIB_SO}: ${OBJS}
	${LD} ${LDFLAGS} ${LDPATH} -o ${LIB_SO} ${OBJS}

clean:
	rm -f ${LIB_SO} ${FS_EXE} *.o

include make.depend
