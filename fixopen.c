/*
 * Copyright (2014) Sandia Corporation. Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive license for use of 
 * this work by or on behalf of the U.S. Government. 
 * NOTICE:
 * For five (5) years from  the United States Government is granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable worldwide license in this data to reproduce, prepare derivative works, and perform publicly and display publicly, by or on behalf of the Government. There is provision for the possible extension of the term of this license. Subsequent to that period or any extension granted, the United States Government is granted for itself and others acting on its behalf a paid-up, nonexclusive, irrevocable worldwide license in this data to reproduce, prepare derivative works, distribute copies to the public, perform publicly and display publicly, and to permit others to do so. The specific term of the license can be identified by inquiry made to Sandia Corporation or DOE.
 * NEITHER THE UNITED STATES GOVERNMENT, NOR THE UNITED STATES DEPARTMENT OF ENERGY, NOR SANDIA CORPORATION, NOR ANY OF THEIR EMPLOYEES, MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY OWNED RIGHTS.
 * Any licensee of this software has the obligation and responsibility to abide by the applicable export control laws, regulations, and general prohibitions relating to the export of technical data. Failure to obtain an export control license or other authority from the Government may result in criminal liability under U.S. laws.
 * 
 * This file is the source code of cuckoohooks.dylib for the darwin analyzer of the Cuckoo sandbox.
 * Using process injection, it hooks system calls of interest to monitor for activity.
 * The full list of syscalls is in sys/syscall.h
 * It is compiled using the commands:
 * gcc -fno-common -c cuckoohooks.c
 * gcc -dynamiclib -o cuckoohooks.dylib cuckoohooks.o
 * or, for a 32-and-64-bit one:
 * gcc -fno-common -c cuckoohooks.c -arch i386
 * gcc -dynamiclib -o cuckoohooks_32.dylib cuckoohooks.o -arch i386
 * gcc -fno-common -c cuckoohooks.c -arch x86_64
 * gcc -dynamiclib -o cuckoohooks_64.dylib cuckoohooks.o -arch x86_64
 * lipo -create cuckoohooks_32.dylib cuckoohooks_64.dylib -output cuckoohooks.dylib
 *
 * A makefile is included for convenience
 * It is injected at runtime into the desired process with:
 * DYLD_FORCE_FLAT_NAMESPACE=1 DYLD_INSERT_LIBRARIES=<path>/cuckoohooks.dylib ./<executable>
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>
#include <time.h>
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <arpa/inet.h>
#include <spawn.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <signal.h>
#include <syslog.h>

#define DYLD_INTERPOSE(_replacment,_replacee) \
__attribute__((used)) static struct{ const void* replacment; const void* replacee; } _interpose_##_replacee \
__attribute__ ((section ("__DATA,__interpose"))) = { (const void*)(unsigned long)&_replacment, (const void*)(unsigned long)&_replacee };

/*
 * This section contains the hooked syscalls.
 */

/*
 * The open(2) syscall hook. The "..." means this function takes an unspecified number of variables.
 * We are mostly interested in files being created and files being written, so that we can inform Cuckoo about them.
 * However, the function will also log all files read, for completeness.
 * https://developer.apple.com/library/mac/documentation/Darwin/Reference/ManPages/man2/open.2.html#//apple_ref/doc/man/2/open
 */

int my_open(const char *path, int oflag,...) {
    // fprintf(stderr, "Starting open within pid %d and file %s and mode %d\n", getpid(), path, oflag);

    // fprintf(stderr, "File %s opened with %d\n", path, oflag); //for debugging
    
    //The most essential unnamed argument is the file permissions, so we need to pass that to the real open
    va_list args;
    va_start(args,oflag); //list of unnamed arguments
    int perm = va_arg(args, int); //permissions
    va_end(args);
   

    //call the real function for the results
    int result;
    int count = 0;
    do {
		if (count > 0)
		{
			fprintf(stderr, "retrying read (errno: %d EINTR: %d)\n", errno, EINTR); //for debugging
		}

		result = open(path, oflag, perm);
		count++;
	} while (result < 0 && errno == EINTR && count <= 1000);

	// fprintf(stderr, "File %s result %d\n", path, result); //for debugging

    return result;
}

DYLD_INTERPOSE(my_open, open);

__attribute__((constructor))
static void customConstructor(int argc, const char **argv)
 {
     fprintf(stderr, "Hello from dylib!\n");
     // syslog(LOG_ERR, "Dylib injection successful in %s\n", argv[0]);
}

