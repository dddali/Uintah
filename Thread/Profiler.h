/*
 * This file was automatically generated by SCC - do NOT edit!
 * You should edit Profiler.scc instead 
 */

#ifndef SCI_THREAD_PROFILER_H
#define SCI_THREAD_PROFILER_H 1



#include "Runnable.h"

#include <stdio.h>
class Profiler  : public Runnable {
    FILE* in;
    FILE* out;
    virtual void run() ;
public:
    Profiler() ;
    Profiler(FILE* in, FILE* out) ;
    ~Profiler() ;
};

#endif
