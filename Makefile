XCFLAGS=${CFLAGS} -std=c99 -Wall -Wextra -pedantic

hypothread: hypothread.c
	${CC} ${XCFLAGS} -o $@ hypothread.c ${LDFLAGS}

clean:
	rm hypothread
