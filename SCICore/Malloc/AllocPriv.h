
/*
 *  AllocPriv.h: ?
 *
 *  Written by:
 *   Author: ?
 *   Department of Computer Science
 *   University of Utah
 *   Date: ?
 *
 *  Copyright (C) 199? SCI Group
 */

#include <stdlib.h>

namespace SCICore {
namespace Malloc {

struct OSHunk;

struct Sentinel {
    unsigned int first_word;
    unsigned int second_word;
};

struct AllocBin;

struct Tag {
//    Allocator* allocator;
//    size_t size;
    AllocBin* bin;
    char* tag;
    Tag* next;
    Tag* prev;
    OSHunk* hunk;
    size_t reqsize;
};

struct AllocBin {
    Tag* free;
    Tag* inuse;
    size_t maxsize;
    size_t minsize;
    int ninuse;
    int ntotal;
    size_t nalloc;
    size_t nfree;
};

struct Allocator {
    unsigned int the_lock;
    void initlock();
    inline void lock();
    void longlock();
    inline void unlock();

    void* alloc_big(size_t size, char* tag);
    
    void* alloc(size_t size, char* tag);
    void free(void*);
    void* realloc(void* p, size_t size);

    int strict;
    int lazy;
    int trace;
    OSHunk* hunks;

    AllocBin* small_bins;
    AllocBin* medium_bins;
    AllocBin big_bin;

    inline AllocBin* get_bin(size_t size);
    void fill_bin(AllocBin*);
    void get_hunk(size_t, OSHunk*&, void*&);

    void init_bin(AllocBin*, size_t maxsize, size_t minsize);

    void audit(Tag*, int);
    size_t obj_maxsize(Tag*);

    // Statistics...
    size_t nalloc;
    size_t nfree;
    size_t sizealloc;
    size_t sizefree;
    size_t nlonglocks;
    size_t nnaps;

    size_t nfillbin;
    size_t nmmap;
    size_t sizemmap;
    size_t nmunmap;
    size_t sizemunmap;

    size_t highwater_alloc;
    size_t highwater_mmap;

    size_t mysize;
};

void AllocError(char*);

} // End namespace Malloc
} // End namespace SCICore

//
// $Log$
// Revision 1.1  1999/07/27 16:56:58  mcq
// Initial commit
//
// Revision 1.3  1999/05/06 19:52:10  dav
// adding .h files back to src tree
//
// Revision 1.1  1999/05/05 21:05:20  dav
// added SCICore .h files to /include directories
//
// Revision 1.1.1.1  1999/04/24 23:12:23  dav
// Import sources
//
//

