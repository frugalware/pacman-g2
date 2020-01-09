FROM fedora:latest

RUN dnf -y install gcc intltool gettext git libtool pkg-config bzip2-devel \
	libarchive doxygen swig vala asciidoc autoconf automake which \
	xz-devel file diffutils libarchive libarchive-devel make \
	perl-ExtUtils-Embed mono-core po4a
WORKDIR /tmp/fst
COPY . /tmp/fst
RUN ./configure --disable-dependency-tracking --disable-python --disable-java --prefix=/usr
RUN make clean
RUN make 
RUN make install
ENTRYPOINT /usr/bin/pacman-g2