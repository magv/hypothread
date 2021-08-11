hypothread: hypothread.c
	${CC} ${CFLAGS} -o $@ hypothread.c ${LDFLAGS}

clean:
	rm hypothread
