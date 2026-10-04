FROM alpine:3.21

RUN apk add --no-cache build-base

WORKDIR /work
COPY . .
RUN make

CMD ["./parallel_sum", "10000000", "5"]
