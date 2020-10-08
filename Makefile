include config.mk

all: star-platinum TAGS

.PHONY: all clean archive

lex.yy.c: lex.l y.tab.c
	${LEX} lex.l

y.tab.c: parse.y
	${YACC} -b y -d parse.y

OBJS = star-platinum.o lex.yy.o y.tab.o
star-platinum: ${OBJS}
	${CC} -o $@ ${OBJS} ${LDFLAGS}

TAGS: star-platinum.c star-platinum.h parse.y lex.l
	-${ETAGS} star-platinum.c star-platinum.h parse.y lex.l || true

clean:
	rm -f star-platinum ${OBJS} lex.yy.c y.tab.c y.tab.h y.output
	rm -f star-platinum.tar.gz
	rm -f TAGS

SRC +=	Makefile config.mk lex.l parse.y star-platinum.c
SRC +=	star-platinum.h star-platinum.1 star-platinum.conf.5
star-platinum.tar.gz: ${SRC}
	rm -f star-platinum.tar.gz
	mkdir star-platinum-archive
	cp ${SRC} ./star-platinum-archive
	tar -czvf star-platinum.tar.gz \
		-s '/^star-platinum-archive/star-platinum/gp' \
		star-platinum-archive
	rm -r star-platinum-archive

archive: star-platinum.tar.gz
