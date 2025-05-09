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
//  File   : StdMeshers_NumberOfSegments_i.hxx
//           Moved here from SMESH_NumberOfSegments_i.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#ifndef _SMESH_NUMBEROFSEGMENTS_I_HXX_
#define _SMESH_NUMBEROFSEGMENTS_I_HXX_

#include "SMESH_StdMeshers_I.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SMESH_Mesh)
#include CORBA_SERVER_HEADER(SMESH_BasicHypothesis)

#include "StdMeshers_Reversible1D_i.hxx"
#include "StdMeshers_NumberOfSegments.hxx"

// ======================================================
// Number of segments hypothesis
// ======================================================
class STDMESHERS_I_EXPORT StdMeshers_NumberOfSegments_i:
  public virtual POA_StdMeshers::StdMeshers_NumberOfSegments,
  public virtual SMESH_Hypothesis_i,
  public virtual StdMeshers_Reversible1D_i
{
public:
  // Constructor
  StdMeshers_NumberOfSegments_i( PortableServer::POA_ptr thePOA,
                                 ::SMESH_Gen*            theGenImpl );
  // Destructor
  virtual ~StdMeshers_NumberOfSegments_i();

  // Builds point distribution according to passed function
  SMESH::double_array* BuildDistributionExpr( const char*, CORBA::Long, CORBA::Long );
  SMESH::double_array* BuildDistributionTab( const SMESH::double_array&, CORBA::Long, CORBA::Long );

  // Set number of segments
  void SetNumberOfSegments( SMESH::smIdType theSegmentsNumber );
  // Get number of segments
  CORBA::Long GetNumberOfSegments();

  // Set distribution type
  void SetDistrType(CORBA::Long typ);
  // Get distribution type
  CORBA::Long GetDistrType();

  // Set scalar factor
  void SetScaleFactor( CORBA::Double theScaleFactor );
  // Get scalar factor
  CORBA::Double GetScaleFactor();

  // Set beta coefficient for Beta Law distribution
  void SetBeta(CORBA::Double beta);
  // Get beta coefficient for Beta Law distribution
  CORBA::Double GetBeta();

  // Set table function for distribution DT_TabFunc
  void SetTableFunction(const SMESH::double_array& table);
  // Get table function for distribution DT_TabFunc
  SMESH::double_array* GetTableFunction();

  // Set expression function for distribution DT_ExprFunc
  void SetExpressionFunction(const char* expr);
  // Get expression function for distribution DT_ExprFunc
  char* GetExpressionFunction();

  // Set the exponent mode on/off
  void SetConversionMode( CORBA::Long conv );
  // Returns true if the exponent mode is set
  CORBA::Long ConversionMode();

  // Get implementation
  ::StdMeshers_NumberOfSegments* GetImpl();
  
  // Verify whether hypothesis supports given entity type 
  CORBA::Boolean IsDimSupported( SMESH::Dimension type );


  // Methods for copying mesh definition to other geometry

  // Return geometry this hypothesis depends on. Return false if there is no geometry parameter
  virtual bool getObjectsDependOn( std::vector< std::string > & entryArray,
                                   std::vector< int >         & subIDArray ) const;

  // Set new geometry instead of that returned by getObjectsDependOn()
  virtual bool setObjectsDependOn( std::vector< std::string > & entryArray,
                                   std::vector< int >         & subIDArray );
 protected:
  virtual std::string getMethodOfParameter(const int paramIndex, int nbVars) const;
};

#endif
