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

//  File   : StdMeshers_ViscousLayers2D_i.cxx
//  Module : SMESH
//
#include "StdMeshers_ViscousLayers2D_i.hxx"

#include "SMESH_Gen.hxx"
#include "SMESH_Gen_i.hxx"
#include "SMESH_PythonDump.hxx"

#include "Utils_CorbaException.hxx"
#include "utilities.h"

#include <TCollection_AsciiString.hxx>

#include CORBA_SERVER_HEADER(SMESH_Group)

using namespace std;

//=============================================================================
/*!
 *  StdMeshers_ViscousLayers2D_i::StdMeshers_ViscousLayers2D_i
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_ViscousLayers2D_i::StdMeshers_ViscousLayers2D_i( PortableServer::POA_ptr thePOA,
                                                            ::SMESH_Gen*            theGenImpl )
  : SALOME::GenericObj_i( thePOA ),
    SMESH_Hypothesis_i( thePOA )
{
  myBaseImpl = new ::StdMeshers_ViscousLayers2D( theGenImpl->GetANewId(),
                                                 theGenImpl );
}

//=============================================================================
/*!
 *  StdMeshers_ViscousLayers2D_i::~StdMeshers_ViscousLayers2D_i
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_ViscousLayers2D_i::~StdMeshers_ViscousLayers2D_i()
{
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::SetEdges(const ::SMESH::long_array& edgeIDs,
                                            CORBA::Boolean             toIgnore)
{
  vector<int> ids( edgeIDs.length() );
  for ( unsigned i = 0; i < ids.size(); ++i )
    if (( ids[i] = edgeIDs[i] ) < 1 )
      THROW_SALOME_CORBA_EXCEPTION( "Invalid edge id", SALOME::BAD_PARAM );

  GetImpl()->SetBndShapes( ids, toIgnore );

  SMESH::TPythonDump() << _this() << ".SetEdges( " << edgeIDs << ", " << toIgnore << " )";
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::SetIgnoreEdges(const ::SMESH::long_array& edgeIDs)
{
  SMESH::TPythonDump pyDump;
  this->SetEdges( edgeIDs, true );
  pyDump<< _this() << ".SetIgnoreEdges( " << edgeIDs << " )";
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

SMESH::long_array* StdMeshers_ViscousLayers2D_i::GetEdges()
{
  vector<int> idsVec = GetImpl()->GetBndShapes();
  SMESH::long_array_var ids = new SMESH::long_array;
  ids->length( idsVec.size() );
  for ( unsigned i = 0; i < idsVec.size(); ++i )
    ids[i] = idsVec[i];
  return ids._retn();
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

SMESH::long_array* StdMeshers_ViscousLayers2D_i::GetIgnoreEdges()
{
  if ( GetImpl()->IsToIgnoreShapes() )
    return this->GetEdges();
  return new SMESH::long_array;
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

CORBA::Boolean StdMeshers_ViscousLayers2D_i::GetIsToIgnoreEdges()
{
  return GetImpl()->IsToIgnoreShapes();
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::SetTotalThickness(::CORBA::Double thickness)
{
  if ( thickness < 1e-100 )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid thickness", SALOME::BAD_PARAM );
  GetImpl()->SetTotalThickness(thickness);
  SMESH::TPythonDump() << _this() << ".SetTotalThickness( " << SMESH::TVar(thickness) << " )";
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

::CORBA::Double StdMeshers_ViscousLayers2D_i::GetTotalThickness()
{
  return GetImpl()->GetTotalThickness();
}

//================================================================================
/*!
 * \brief 
 *  \param nb - 
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::SetNumberLayers(::CORBA::Short nb)
{
  if ( nb < 1 )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid number of layers", SALOME::BAD_PARAM );
  GetImpl()->SetNumberLayers( nb );
  SMESH::TPythonDump() << _this() << ".SetNumberLayers( " << SMESH::TVar(nb) << " )";
}

//================================================================================
/*!
 * \brief 
 */
//================================================================================

::CORBA::Short StdMeshers_ViscousLayers2D_i::GetNumberLayers()
{
  return CORBA::Short( GetImpl()->GetNumberLayers() );
}

//================================================================================
/*!
 * \brief 
 *  \param factor - 
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::SetStretchFactor(::CORBA::Double factor)
{
  if ( factor < 1 )
    THROW_SALOME_CORBA_EXCEPTION( "Invalid stretch factor, it must be >= 1.0", SALOME::BAD_PARAM );
  GetImpl()->SetStretchFactor(factor);
  SMESH::TPythonDump() << _this() << ".SetStretchFactor( " << SMESH::TVar(factor) << " )";
}

//================================================================================
/*!
 * \brief 
 * 
 */
//================================================================================

::CORBA::Double StdMeshers_ViscousLayers2D_i::GetStretchFactor()
{
  return GetImpl()->GetStretchFactor();
}

//================================================================================
/*!
 * \brief Set name of a group of layers elements
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::SetGroupName(const char* name)
{
  if ( GetImpl()->GetGroupName() != name )
  {
    GetImpl()->SetGroupName( name );
    SMESH::TPythonDump() << _this() << ".SetGroupName( '" << name << "' )";
  }
}

//================================================================================
/*!
 * \brief Return name of a group of layers elements
 */
//================================================================================

char* StdMeshers_ViscousLayers2D_i::GetGroupName()
{
  return CORBA::string_dup( GetImpl()->GetGroupName().c_str() );
}


//=============================================================================
/*!
 *  Get implementation
 */
//=============================================================================

::StdMeshers_ViscousLayers2D* StdMeshers_ViscousLayers2D_i::GetImpl()
{
  return ( ::StdMeshers_ViscousLayers2D* )myBaseImpl;
}

//================================================================================
/*!
 * \brief Verify whether hypothesis supports given entity type
 *  \param type - dimension (see SMESH::Dimension enumeration)
 *  \retval CORBA::Boolean - TRUE if dimension is supported, FALSE otherwise
 *
 * Verify whether hypothesis supports given entity type (see SMESH::Dimension enumeration)
 */
//================================================================================

CORBA::Boolean StdMeshers_ViscousLayers2D_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_2D;
}

//================================================================================
/*!
 * \brief Sets sub-mesh event listeners to clear sub-meshes of edges
 *        shrunk by viscous layers
 */
//================================================================================

void StdMeshers_ViscousLayers2D_i::UpdateAsMeshesRestored()
{
  GetImpl()->RestoreListeners();
}

//================================================================================
/*!
 * \brief Return geometry this hypothesis depends on. Return false if there is no geometry parameter
 */
//================================================================================

bool
StdMeshers_ViscousLayers2D_i::getObjectsDependOn( std::vector< std::string > & /*entryArray*/,
                                                  std::vector< int >         & subIDArray ) const
{
  const ::StdMeshers_ViscousLayers2D* impl =
    static_cast<const ::StdMeshers_ViscousLayers2D*>( myBaseImpl );

  subIDArray = impl->GetBndShapes();

  return true;
}

//================================================================================
/*!
 * \brief Set new geometry instead of that returned by getObjectsDependOn()
 */
//================================================================================

bool
StdMeshers_ViscousLayers2D_i::setObjectsDependOn( std::vector< std::string > & /*entryArray*/,
                                                  std::vector< int >         & subIDArray )
{
  std::vector< int > newIDs;
  newIDs.reserve( subIDArray.size() );

  for ( size_t i = 0; i < subIDArray.size(); ++i )
    if ( subIDArray[ i ] > 0 )
      newIDs.push_back( subIDArray[ i ]);

  GetImpl()->SetBndShapes( newIDs, GetIsToIgnoreEdges() );

  return true;
}
