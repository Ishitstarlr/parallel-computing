FROM alpine:3.21

RUN apk add --no-cache build-base

WORKDIR /work
COPY . .
RUN make -C task1 && make -C task2

CMD ["./task1/parallel_sum", "10000000", "5"]
