FROM alpine:3.8 AS build
RUN apk add alpine-sdk
RUN apk add clang upx binutils-gold
RUN apk add python3
WORKDIR /tmp
RUN curl -o ek.tgz http://www.muppetlabs.com/~breadbox/pub/software/ELFkickers-3.1.tar.gz
RUN tar xzf ek.tgz
RUN cd ELFkickers-* && make install PROGRAMS=sstrip
WORKDIR /build
ADD ./diced.c diced.c
RUN clang -fno-exceptions -fno-asynchronous-unwind-tables -fuse-ld=gold -static -Wall -Os -s diced.c -o diced
# add `-no-integrated-as -Xassembler -adhln` above for assembly
RUN strip --strip-all -R .comment -R .gnu.hash -R .note.stapsdt -R .note.gnu.gold-version ./diced
ADD ./mclean.py mclean.py
RUN python3 mclean.py ./diced
RUN sstrip -z ./diced
RUN objdump -s -x ./diced && size ./diced && file ./diced && ls -la .
RUN upx --best ./diced && ls -la .
FROM scratch
COPY --from=build /build/diced /diced
ENV PORT 8000
CMD /diced