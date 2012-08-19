SOURCES := common.c fetch.c file.c ftp.c http.c
OBJECTS := $(patsubst %.c,%.o,$(SOURCES))

CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -DINET6 -DWITH_SSL -DNETBSD
CFLAGS += $(shell pkg-config --cflags libssl)

%.o: %.c ftperr.h httperr.h
	$(CC) $(CFLAGS) -c $< -o $@

libfetch.a: $(OBJECTS)
	$(AR) rcs $@ $^

ftperr.h: errlist.sh ftp.errors
	./errlist.sh ftp_errlist FTP ftp.errors > ftperr.h

httperr.h: errlist.sh http.errors
	./errlist.sh http_errlist HTTP http.errors > httperr.h

clean:
	$(RM) ftperr.h httperr.h $(OBJECTS) libfetch.a
