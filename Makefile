all: ajl ajl.o ajlparse.o ajlcurl.o

ajlparse.o: ajlparse.c ajlparse.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajlparse.o ajlparse.c -std=gnu99

ajl.o: ajlparse.c ajlparse.h ajl.c ajl.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajl.o ajl.c -std=gnu99

ajlcurl.o: ajlparse.c ajlparse.h ajl.c ajl.h ajlcurl.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajlcurl.o ajl.c -std=gnu99 -lcurl -DJCURL

ajl: ajlparse.c ajlparse.h ajl.c ajl.h Makefile
	cc -g -Wall -Wextra -O -o ajl ajl.c -std=gnu99 -lcurl -DJCURL

clean:
	rm -f *.o ajl
