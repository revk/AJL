all: ajl ajl.o ajlparse.o

axlparse.o: ajlparse.c ajlparse.h
	cc -g -Wall -Wextra -DLIB -O -c -o ajlparse.o ajlparse.c

axl.o: ajlparse.c ajlparse.h ajl.c ajl.h
	cc -g -Wall -Wextra -DLIB -O -c -o ajl.o ajl.c ajlparse.c

axl: ajlparse.c ajlparse.h ajl.c ajl.h
	cc -g -Wall -Wextra -O -o ajl ajl.c ajlparse.c

clean:
	rm -f *.o ajl
