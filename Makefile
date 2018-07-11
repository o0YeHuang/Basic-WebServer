
CC=g++
CFLAGS=-ggdb -Wall -Wextra -pedantic -Werror
DEPS = httpd.h Practical.h HTTPMessage.hpp HTTPRequest.hpp HTTPResponse.hpp
SRCS = httpd.cpp DieWithMessage.cpp
MAIN_SRCS = main.c $(SRCS)
MAIN_OBJS = $(MAIN_SRCS:.c=.o)

default: httpd

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

httpd:    $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o httpd $(MAIN_OBJS) -lpthread

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f httpd *.o