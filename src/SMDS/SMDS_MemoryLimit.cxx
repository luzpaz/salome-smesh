// Copyright (C) 2007-2025  CEA, EDF, OPEN CASCADE
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
//
// See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
//

// File      : SMDS_MemoryLimit.cxx
// Created   : Fri Sep 21 17:16:42 2007
// Author    : Edward AGAPOV (eap)
// Executable to find out a lower RAM limit (MB), i.e. at what size of freeRAM
// reported by sysinfo, no more memory can be allocated.
// This is not done inside a function of SALOME because allocated memory is not always
// returned to the system. (PAL16631)
//
#if !defined WIN32 && !defined __APPLE__
#include <sys/sysinfo.h>
#endif

#include <iostream>

int main ()
{
  // To better understand what is going on here, consult bug [SALOME platform 0019911]
#if !defined WIN32 && !defined __APPLE__
  struct sysinfo si;
  int err = sysinfo( &si );
  if ( err )
    return -1;
  unsigned long freeRamKb = ( si.freeram  * si.mem_unit ) / 1024;

  // total RAM size in Gb, float is in order not to have 1 instead of 1.9
  float totalramGb = float( si.totalram * si.mem_unit ) / 1024 / 1024 / 1024;

  // nb Kbytes to allocate at one step. Small nb leads to hung up
  const int stepKb = int( 5 * totalramGb );

  unsigned long nbSteps = freeRamKb / stepKb * 2;
  try {
    while ( nbSteps-- ) {
      new char[stepKb*1024];
      err = sysinfo( &si );
      if ( !err )
        freeRamKb = ( si.freeram  * si.mem_unit ) / 1024;
    }
  } catch (...) {}

// if (SALOME::VerbosityActivated())
//  std::cout << freeRamKb / 1024 << std::endl;

  return freeRamKb / 1024;

#endif

  return -1;
}
