LIBALPMINCDIR = ../../lib/libalpm

CFLAGS += $(shell python -c 'from distutils import sysconfig; print "-I" + sysconfig.get_python_inc()') -I$(LIBALPMINCDIR)
ifeq ($(shell arch),x86_64)
CFLAGS += -fPIC
endif
LDFLAGS += -lalpm
LIBDIR += $(shell python -c 'from distutils import sysconfig; print sysconfig.get_python_lib()')

all: _alpm.so alpm.pyc

_alpm.so: alpm_wrap.o
	$(CC) -shared -Wl,-soname,$@ -o $@ $^ $(LDFLAGS)

alpm_wrap.o: alpm_wrap.c
	$(CC) $(CFLAGS) -c -o $@ -include alpm.h $^

alpm_wrap.c:
	cp $(LIBALPMINCDIR)/alpm.h ./
	swig -python -module alpm alpm.h
	# strip the unnecessary prefixes
	sed -i 's/^alpm_//;s/^PM_//' alpm.py

alpm.pyc: alpm.py
	python -c "import compileall; compileall.compile_dir('.',1,'.')"

install: _alpm.so alpm.py alpm.pyc
	mkdir -p $(DESTDIR)$(LIBDIR)
	install $^ $(DESTDIR)$(LIBDIR)

clean:
	rm -f _alpm* alpm*
