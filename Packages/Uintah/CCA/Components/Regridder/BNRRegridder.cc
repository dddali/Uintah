#include <Packages/Uintah/CCA/Components/Regridder/BNRRegridder.h>
#include <Packages/Uintah/Core/Grid/Grid.h>

using namespace Uintah;

BNRRegridder::BNRRegridder(ProcessorGroup* pg) : RegridderCommon(pg)
{

}

BNRRegridder::~BNRRegridder()
{

}

Grid* BNRRegridder::regrid(Grid* oldgrid, SchedulerP sched)
{
  return oldgrid;
}
