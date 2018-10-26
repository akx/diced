FROM alpine:3.8 AS build
RUN apk add alpine-sdk
WORKDIR /build
ADD ./diced.c diced.c
RUN gcc -static -o diced -Wall -Os diced.c && strip diced && ls -laR /build
FROM scratch
COPY --from=build /build/diced /diced
ENV PORT 8000
CMD /diced