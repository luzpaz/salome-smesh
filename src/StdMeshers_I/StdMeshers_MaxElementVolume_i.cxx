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

//  SMESH SMESH_I : idl implementation based on 'SMESH' unit's classes
//  File   : StdMeshers_MaxElementVolume_i.cxx
//           Moved here from SMESH_MaxElementVolume_i.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_MaxElementVolume_i.hxx"
#include "SMESH_Gen_i.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_PythonDump.hxx"

#include "Utils_CorbaException.hxx"
#include "utilities.h"

#include <TCollection_AsciiString.hxx>

using namespace std;

//=============================================================================
/*!
 *  StdMeshers_MaxElementVolume_i::StdMeshers_MaxElementVolume_i
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_MaxElementVolume_i::StdMeshers_MaxElementVolume_i( PortableServer::POA_ptr thePOA,
                                                              ::SMESH_Gen*            theGenImpl )
     : SALOME::GenericObj_i( thePOA ), 
       SMESH_Hypothesis_i( thePOA )
{
  myBaseImpl = new ::StdMeshers_MaxElementVolume( theGenImpl->GetANewId(),
                                                  theGenImpl );
}

//=============================================================================
/*!
 *  StdMeshers_MaxElementVolume_i::~StdMeshers_MaxElementVolume_i
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_MaxElementVolume_i::~StdMeshers_MaxElementVolume_i()
{
}

//=============================================================================
/*!
 *  StdMeshers_MaxElementVolume_i::SetMaxElementVolume
 *
 *  Set maximum element volume 
 */
//=============================================================================

void StdMeshers_MaxElementVolume_i::SetMaxElementVolume( CORBA::Double theVolume )
{
  ASSERT( myBaseImpl );
  try {
    this->GetImpl()->SetMaxVolume( theVolume );
  }
  catch (SALOME_Exception& S_ex) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }

  // Update Python script
  SMESH::TPythonDump() << _this() << ".SetMaxElementVolume( " << SMESH::TVar(theVolume) << " )";
}

//=============================================================================
/*!
 *  StdMeshers_MaxElementVolume_i::GetMaxElementVolume
 *
 *  Get maximum element volume 
 */
//=============================================================================

CORBA::Double StdMeshers_MaxElementVolume_i::GetMaxElementVolume()
{
  ASSERT( myBaseImpl );
  return this->GetImpl()->GetMaxVolume();
}

//=============================================================================
/*!
 *  StdMeshers_MaxElementVolume_i::GetImpl
 *
 *  Get implementation
 */
//=============================================================================

::StdMeshers_MaxElementVolume* StdMeshers_MaxElementVolume_i::GetImpl()
{
  return ( ::StdMeshers_MaxElementVolume* )myBaseImpl;
}

//================================================================================
/*!
 * \brief Verify whether hypothesis supports given entity type 
  * \param type - dimension (see SMESH::Dimension enumeration)
  * \retval CORBA::Boolean - TRUE if dimension is supported, FALSE otherwise
 * 
 * Verify whether hypothesis supports given entity type (see SMESH::Dimension enumeration)
 */
//================================================================================  
CORBA::Boolean StdMeshers_MaxElementVolume_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_3D;
}

//================================================================================
/*!
 * \brief Return method name corresponding to index of variable parameter
 */
//================================================================================

std::string StdMeshers_MaxElementVolume_i::getMethodOfParameter(const int, int) const
{
  return "SetMaxElementVolume";
}
