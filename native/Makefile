SRCS=fuse_callback.c javafs.c javafs_bindings.c native_impl.c util.c
OBJS=fuse_callback.o javafs.o javafs_bindings.o native_impl.o util.o

include make.flags

DEPEND=makedepend $(DEPFLAGS)

all: ${FS_EXE} ${LIB_SO}

dep:
	${DEPEND} ${INCLUDES} -fmake.depend ${SRCS}

.c.o:
	${CC} -c ${CFLAGS} ${INCLUDES} $*.c

${FS_EXE}: ${OBJS}
	${CC} -o ${FS_EXE} ${OBJS} ${CFLAGS} ${LDFLAGS} ${LDPATH} ${RPATH}

${LIB_SO}: ${OBJS}
	${LD} -o ${LIB_SO} ${OBJS} ${LDFLAGS} ${LDPATH}

clean:
	rm -f ${LIB_SO} ${FS_EXE} *.o

include make.depend
