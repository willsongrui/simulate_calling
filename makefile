PREFIX=${CINDIR}
CC=g++
SOURCE=simulate.cpp log.cpp
OBJ=${SOURCE:.cpp=.o}
OUT=${PREFIX}/bin/scfagent
SLP=${PREFIX}/etc/mscontrol.bin

INCLUDE=-I${CINDIR}/include -Iinc 
CFLAGS=-g -Wall -O3 -c
LIBS=-L ${CINDIR}/lib -lscf_nodatabase -lpthread -lnsl -ldl -lcrypt -llua
all:${OUT}
	@echo completed.

${OUT}:${OBJ}
	${CC} -o $@ ${OBJ} ${LIBS} 
	
.cpp.o:
	${CC} -o $@ $< ${CFLAGS} ${INCLUDE}

clean:
	rm -f ${OBJ}
	rm -f ${OUT}
	make -C slp clean
