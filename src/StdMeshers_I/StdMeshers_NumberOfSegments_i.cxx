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
//  File   : StdMeshers_NumberOfSegments_i.cxx
//           Moved here from SMESH_NumberOfSegments_i.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_NumberOfSegments_i.hxx"
#include "SMESH_Gen_i.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_PythonDump.hxx"

#include "Utils_CorbaException.hxx"
#include "utilities.h"

#include <TCollection_AsciiString.hxx>

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::StdMeshers_NumberOfSegments_i
 *
 *  Constructor
 */
//=============================================================================

StdMeshers_NumberOfSegments_i::StdMeshers_NumberOfSegments_i( PortableServer::POA_ptr thePOA,
                                                              ::SMESH_Gen*            theGenImpl )
  : SALOME::GenericObj_i( thePOA ),
    SMESH_Hypothesis_i( thePOA ),
    StdMeshers_Reversible1D_i( this )
{
  myBaseImpl = new ::StdMeshers_NumberOfSegments( theGenImpl->GetANewId(),
                                                  theGenImpl );
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::~StdMeshers_NumberOfSegments_i
 *
 *  Destructor
 */
//=============================================================================

StdMeshers_NumberOfSegments_i::~StdMeshers_NumberOfSegments_i()
{
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::BuildDistribution
 *
 *  Builds point distribution according to passed function
 */
//=============================================================================
SMESH::double_array* StdMeshers_NumberOfSegments_i::BuildDistributionExpr( const char* func, 
                                                                           CORBA::Long nbSeg, 
                                                                           CORBA::Long conv )
{
  ASSERT( myBaseImpl );
  try
  {
    SMESH::double_array_var aRes = new SMESH::double_array();
    const std::vector<double>& res = this->GetImpl()->BuildDistributionExpr( func, nbSeg, conv );
    aRes->length( res.size() );
    for (size_t i = 0; i < res.size(); i++)
      aRes[i] = res[i];
    return aRes._retn();
  }
  catch( SALOME_Exception& S_ex )
  {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(), SALOME::BAD_PARAM );
  }
}

SMESH::double_array*
StdMeshers_NumberOfSegments_i::BuildDistributionTab( const SMESH::double_array& func,
                                                     CORBA::Long                nbSeg,
                                                     CORBA::Long                conv )
{
  ASSERT( myBaseImpl );

  std::vector<double> tbl( func.length() );
  for ( size_t i = 0; i < tbl.size(); i++ )
    tbl[i] = func[i];

  try
  {
    SMESH::double_array_var   aRes = new SMESH::double_array();
    const std::vector<double>& res = this->GetImpl()->BuildDistributionTab( tbl, nbSeg, conv );
    aRes->length( res.size() );
    for (size_t i = 0; i < res.size(); i++)
      aRes[i] = res[i];
    return aRes._retn();
  }
  catch( SALOME_Exception& S_ex )
  {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(), SALOME::BAD_PARAM );
  }
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::SetNumberOfSegments
 *
 *  Set number of segments
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetNumberOfSegments( SMESH::smIdType theSegmentsNumber )
{
  ASSERT( myBaseImpl );
  try {
    this->GetImpl()->SetNumberOfSegments( theSegmentsNumber );
  }
  catch (SALOME_Exception& S_ex) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }

  // Update Python script
  SMESH::TPythonDump() << _this() << ".SetNumberOfSegments( " << SMESH::TVar(theSegmentsNumber) << " )";
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::GetNumberOfSegments
 *
 *  Get number of segments
 */
//=============================================================================

CORBA::Long StdMeshers_NumberOfSegments_i::GetNumberOfSegments()
{
  ASSERT( myBaseImpl );
  return this->GetImpl()->GetNumberOfSegments();
}

//=============================================================================
/*!
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetDistrType(CORBA::Long typ)
{
  ASSERT( myBaseImpl );
  try {
    CORBA::Long oldType = (CORBA::Long) this->GetImpl()->GetDistrType();

    this->GetImpl()->SetDistrType( (::StdMeshers_NumberOfSegments::DistrType) typ );

    // Update Python script
    if ( oldType != typ )
      SMESH::TPythonDump() << _this() << ".SetDistrType( " << typ << " )";
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
}

//=============================================================================
/*!
 */
//=============================================================================

CORBA::Long StdMeshers_NumberOfSegments_i::GetDistrType()
{
  ASSERT( myBaseImpl );
  return this->GetImpl()->GetDistrType();
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::SetScaleFactor
 *
 *  Set scalar factor
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetScaleFactor( CORBA::Double theScaleFactor )
{
  ASSERT( myBaseImpl );
  try {
    this->GetImpl()->SetScaleFactor( theScaleFactor );
    // Update Python script
    SMESH::TPythonDump() << _this() << ".SetScaleFactor( " << SMESH::TVar(theScaleFactor) << " )";
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::GetScaleFactor
 *
 *  Get scalar factor
 */
//=============================================================================

CORBA::Double StdMeshers_NumberOfSegments_i::GetScaleFactor()
{
  ASSERT( myBaseImpl );
  double scale;
  try {
    scale = this->GetImpl()->GetScaleFactor();
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
  return scale;
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::SetBeta
 *
 *  Set beta coefficient for Beta Law distribution
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetBeta(CORBA::Double beta)
{
  ASSERT(myBaseImpl);
  try {
    this->GetImpl()->SetBeta(beta);
    // Update Python script
    SMESH::TPythonDump() << _this() << ".SetBeta( " << SMESH::TVar(beta) << " )";
  }
  catch (SALOME_Exception& S_ex) {
    THROW_SALOME_CORBA_EXCEPTION(S_ex.what(), SALOME::BAD_PARAM);
  }
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::GetBeta
 *
 *  Get beta coefficient for Beta Law distribution
 */
//=============================================================================

CORBA::Double StdMeshers_NumberOfSegments_i::GetBeta()
{
  ASSERT(myBaseImpl);

  double beta = 1.0;
  try {
    beta = this->GetImpl()->GetBeta();
  }
  catch (SALOME_Exception& S_ex) {
    THROW_SALOME_CORBA_EXCEPTION(S_ex.what(), SALOME::BAD_PARAM);
  }

  return beta;
}

//=============================================================================
/*!
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetTableFunction(const SMESH::double_array& table)
{
  ASSERT( myBaseImpl );
  std::vector<double> tbl( table.length() );
  for ( CORBA::ULong i = 0; i < table.length(); i++)
    tbl[i] = table[i];
  try {
    this->GetImpl()->SetTableFunction( tbl );
    // Update Python script
    SMESH::TPythonDump() << _this() << ".SetTableFunction( " << table << " )";
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
}

//=============================================================================
/*!
 */
//=============================================================================

SMESH::double_array* StdMeshers_NumberOfSegments_i::GetTableFunction()
{
  ASSERT( myBaseImpl );
  const std::vector<double>* tbl;
  try {
    tbl = &this->GetImpl()->GetTableFunction();
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(), SALOME::BAD_PARAM );
  }
  SMESH::double_array_var aRes = new SMESH::double_array();
  aRes->length(tbl->size());
  for ( size_t i = 0; i < tbl->size(); i++ )
    aRes[i] = (*tbl)[i];
  return aRes._retn();
}

//=============================================================================
/*!
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetExpressionFunction(const char* expr)
{
  ASSERT( myBaseImpl );
  try {
    this->GetImpl()->SetExpressionFunction( expr );
    // Update Python script
    SMESH::TPythonDump() << _this() << ".SetExpressionFunction( '" << expr << "' )";
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
}

//=============================================================================
/*!
 */
//=============================================================================

char* StdMeshers_NumberOfSegments_i::GetExpressionFunction()
{
  ASSERT( myBaseImpl );
  const char* expr;
  try {
    expr = this->GetImpl()->GetExpressionFunction();
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
  return CORBA::string_dup(expr);
}

//=============================================================================
/*!
 */
//=============================================================================

void StdMeshers_NumberOfSegments_i::SetConversionMode(CORBA::Long conv )
{
  ASSERT( myBaseImpl );
  try {
    this->GetImpl()->SetConversionMode( conv );
    // Update Python script
    SMESH::TPythonDump() << _this() << ".SetConversionMode( " << conv << " )";
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
}

//=============================================================================
/*!
 */
//=============================================================================

CORBA::Long StdMeshers_NumberOfSegments_i::ConversionMode()
{
  ASSERT( myBaseImpl );
  int conv;
  try {
    conv = this->GetImpl()->ConversionMode();
  }
  catch ( SALOME_Exception& S_ex ) {
    THROW_SALOME_CORBA_EXCEPTION( S_ex.what(),
                                  SALOME::BAD_PARAM );
  }
  return conv;
}

//=============================================================================
/*!
 *  StdMeshers_NumberOfSegments_i::GetImpl
 *
 *  Get implementation
 */
//=============================================================================

::StdMeshers_NumberOfSegments* StdMeshers_NumberOfSegments_i::GetImpl()
{
  return ( ::StdMeshers_NumberOfSegments* )myBaseImpl;
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
CORBA::Boolean StdMeshers_NumberOfSegments_i::IsDimSupported( SMESH::Dimension type )
{
  return type == SMESH::DIM_1D;
}

//================================================================================
/*!
 * \brief Return method name corresponding to index of variable parameter
 */
//================================================================================

std::string StdMeshers_NumberOfSegments_i::getMethodOfParameter(const int paramIndex, int ) const
{
  return paramIndex == 0 ? "SetNumberOfSegments" : "SetScaleFactor";
}


//================================================================================
/*!
 * \brief Return geometry this hypothesis depends on. Return false if there is no geometry parameter
 */
//================================================================================

bool
StdMeshers_NumberOfSegments_i::getObjectsDependOn( std::vector< std::string > & entryArray,
                                                   std::vector< int >         & subIDArray ) const
{
  return StdMeshers_Reversible1D_i::getObjectsDependOn( entryArray, subIDArray );
}

//================================================================================
/*!
 * \brief Set new geometry instead of that returned by getObjectsDependOn()
 */
//================================================================================

bool
StdMeshers_NumberOfSegments_i::setObjectsDependOn( std::vector< std::string > & entryArray,
                                                   std::vector< int >         & subIDArray )
{
  return StdMeshers_Reversible1D_i::setObjectsDependOn( entryArray, subIDArray );
}
