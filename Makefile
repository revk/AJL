all: ajl ajl.o ajlparse.o ajlcurl.o jcgi.o jcgi

ajlparse.o: ajlparse.c ajlparse.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajlparse.o ajlparse.c -std=gnu99 -D_GNU_SOURCE

ajl.o: ajlparse.c ajlparse.h ajl.c ajl.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajl.o ajl.c -std=gnu99 -D_GNU_SOURCE

ajlcurl.o: ajlparse.c ajlparse.h ajl.c ajl.h ajlcurl.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o ajlcurl.o ajl.c -std=gnu99 -D_GNU_SOURCE -lcurl -DJCURL

ajl: ajlparse.c ajlparse.h ajl.c ajl.h Makefile
	cc -g -Wall -Wextra -O -o ajl ajl.c -std=gnu99 -D_GNU_SOURCE -lcurl -DJCURL -lpopt

jcgi.o: jcgi.c ajl.h jcgi.h ajlparse.h Makefile
	cc -g -Wall -Wextra -DLIB -O -c -o jcgi.o jcgi.c -std=gnu99 -D_GNU_SOURCE

jcgi: jcgi.c ajl.o jcgi.h Makefile
	cc -g -Wall -Wextra -O -o jcgi jcgi.c ajl.o -std=gnu99 -D_GNU_SOURCE -lpopt

clean:
	rm -f *.o ajl
