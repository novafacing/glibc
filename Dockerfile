# NOTE: We use Fedora 35 because it has a recent enough gcc
# but is old enough to carry Make 4.3<, which is required because
# Make 4.4 has an infinite loop on the glibc makefiles as outlined
# here: https://github.com/crosstool-ng/crosstool-ng/issues/1932
# here: https://github.com/crosstool-ng/crosstool-ng/pull/1990
FROM fedora:35

RUN dnf -y update && \
    dnf -y install gcc g++ make autoconf autoconf-archive texinfo

COPY . /glibc

WORKDIR /glibc/build

RUN mkdir /install && \
    ../configure --prefix=/install --exec-prefix=/install && \
    make && \
    make install && \
    make clean
