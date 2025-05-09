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

// File   : StdMeshersGUI.cxx
// Author : Alexander SOLOVYOV, Open CASCADE S.A.S.
// SMESH includes
//
#include "StdMeshersGUI_BlockRenumberCreator.h"
#include "StdMeshersGUI_CartesianParamCreator.h"
#include "StdMeshersGUI_NbSegmentsCreator.h"
#include "StdMeshersGUI_QuadrangleParamWdg.h"
#include "StdMeshersGUI_StdHypothesisCreator.h"

//=============================================================================
/*! GetHypothesisCreator
 *
 */
//=============================================================================
extern "C"
{
 STDMESHERSGUI_EXPORT
  SMESHGUI_GenericHypothesisCreator* GetHypothesisCreator( const QString& aHypType )
  {
    if( aHypType=="NumberOfSegments" )
      return new StdMeshersGUI_NbSegmentsCreator();
    else if ( aHypType=="CartesianParameters3D" )
      return new StdMeshersGUI_CartesianParamCreator( aHypType );
    else if ( aHypType=="QuadrangleParams" )
      return new StdMeshersGUI_QuadrangleParamCreator( aHypType );
    else if ( aHypType=="BlockRenumber")
      return new StdMeshersGUI_BlockRenumberCreator( aHypType );
    else
      return new StdMeshersGUI_StdHypothesisCreator( aHypType );
  }
}
