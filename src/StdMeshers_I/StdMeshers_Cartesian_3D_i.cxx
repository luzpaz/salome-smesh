// Copyright (C) 2007-2025  CEA, EDF, OPEN CASCADE
//
// Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
// CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
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
//  File   : StdMeshers_Cartesian_3D_i.cxx
//  Module : SMESH
//
#include "StdMeshers_Cartesian_3D_i.hxx"
#include "StdMeshers_Cartesian_3D.hxx"
#include "SMESH_Gen.hxx"

#include "Utils_CorbaException.hxx"
#include "utilities.h"

//=============================================================================
/*!
 *  StdMeshers_Cartesian_3D_i::StdMeshers_Cartesian_3D_i
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_Cartesian_3D_i::StdMeshers_Cartesian_3D_i( PortableServer::POA_ptr thePOA,
                                                      ::SMESH_Gen*            theGenImpl )
  : SALOME::GenericObj_i( thePOA ), 
    SMESH_Hypothesis_i( thePOA ), 
    SMESH_Algo_i( thePOA ),
    SMESH_3D_Algo_i( thePOA )
{
  myBaseImpl = new ::StdMeshers_Cartesian_3D( theGenImpl->GetANewId(),
                                              theGenImpl );
}

//=============================================================================
/*!
 *  StdMeshers_Cartesian_3D_i::~StdMeshers_Cartesian_3D_i
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_Cartesian_3D_i::~StdMeshers_Cartesian_3D_i()
{
}

//=============================================================================
/*!
 *  StdMeshers_Cartesian_3D_i::GetImpl
 *
 *  Get implementation
 */
//=============================================================================

::StdMeshers_Cartesian_3D* StdMeshers_Cartesian_3D_i::GetImpl()
{
  return ( ::StdMeshers_Cartesian_3D* )myBaseImpl;
}
