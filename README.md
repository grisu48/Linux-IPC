# Linux-IPC

[![Build Status](https://travis-ci.org/grisu48/Linux-IPC.svg?branch=master)](https://travis-ci.org/grisu48/Linux-IPC)

Templates and concepts for Linux IPC

## Shared memory

The `SharedMemory` class provides a wrapper class around System V shared memory segments.

### Usage

To use System V Shared memory, you need a common ID to identify which shared memory the individual processes attach to. This ID needs to be known across the individual processes.

When the key `IPC_KEY` is known, create a shared memory is easy as

    size_t size = sizeof(double) * 8;		// Want to create array of 8 double
    SharedMemory shm(IPC_KEY, size);
    double *array = (double*)shm.get();			// This is your array
    // Whatever you want to do
    memset(array, 'a', 8);

Once the SharedMemory instance is deleted, also the shared memory segment will be deleted!

See also `example.cpp`