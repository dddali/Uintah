/*
 *  SigmaSet.h: Set of sigmas (e.g. conductivies) for finite-elements
 *
 *  Written by:
 *   David Weinstein
 *   Department of Computer Science
 *   University of Utah
 *   January 1996
 *
 *  Copyright (C) 1996 SCI Group
 */

#include <Datatypes/SigmaSet.h>

#include <Malloc/Allocator.h>
#include <iostream.h>
#include <fstream.h>

static Persistent* make_SigmaSet()
{
    return scinew SigmaSet;
}

PersistentTypeID SigmaSet::type_id("SigmaSet", "Datatype", make_SigmaSet);

SigmaSet::SigmaSet()
{
}

SigmaSet::SigmaSet(const SigmaSet& copy)
: names(copy.names), vals(copy.vals)
{
}

SigmaSet::SigmaSet(int nsigs, int vals_per_sig)
: names(nsigs), vals(nsigs, vals_per_sig)
{
}

SigmaSet::~SigmaSet()
{
}

SigmaSet* SigmaSet::clone()
{
    return scinew SigmaSet(*this);
}

#define SIGMASET_VERSION 1

void SigmaSet::io(Piostream& stream)
{
    stream.begin_class("SigmaSet", SIGMASET_VERSION);
    Pio(stream, names);
    Pio(stream, vals);
    stream.end_class();
}

