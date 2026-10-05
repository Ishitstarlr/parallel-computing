# Parallel Computing

This repository contains my attempts at two tasks from the WebClub NITK Systems
& Security SIG Parallel Computing recruitment task. Each task is self-contained
inside its own folder.

```text
task1/  Array sum using one thread and two four-thread strategies
task2/  Lock-free single-producer/single-consumer ring buffer
```

## Task folders

- [Task I — array-sum comparison](task1/README.md)
- [Task II — lock-free SPSC ring buffer](task2/README.md)

## Run in Linux through Docker

Docker is used so the same Linux build works from macOS, Windows, or Linux.

```sh
docker build -t parallel-computing-linux .
```

Run Task I:

```sh
docker run --rm parallel-computing-linux ./task1/parallel_sum 10000000 5
```

Run Task II:

```sh
docker run --rm parallel-computing-linux ./task2/spsc_ring 20
```

The `--rm` flag removes the temporary container after the program finishes. It
does not remove the Docker image or source code.
