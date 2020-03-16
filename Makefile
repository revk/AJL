all: ajl ajl.o ajlparse.o

ajlparse.o: ajlparse.c ajlparse.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajlparse.o ajlparse.c

ajl.o: ajlparse.c ajlparse.h ajl.c ajl.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajl.o ajl.c

ajl: ajlparse.c ajlparse.h ajl.c ajl.h Makefile
	cc -g -Wall -Wextra -O -o ajl ajl.c

clean:
	rm -f *.o ajl
