// Copyright (C) 2010-2025  CEA, EDF, OPEN CASCADE
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

//  SMESH SMDS : implementation of Salome mesh data structure
//  File   : SMDS_MeshEdge.hxx
//  Module : SMESH
//
#ifndef _SMDS_MeshEdge_HeaderFile
#define _SMDS_MeshEdge_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_MeshCell.hxx"

/*!
 * \brief Edge mesh element. This type is not allocated.
 *        It is only used as function argument type to provide more clear semantic.
 */
class SMDS_EXPORT SMDS_MeshEdge: public SMDS_MeshCell
{
 public:
  virtual SMDSAbs_ElementType  GetType()     const { return SMDSAbs_Edge; }
  virtual SMDSAbs_GeometryType GetGeomType() const { return SMDSGeom_EDGE; }

  static SMDSAbs_ElementType Type() { return SMDSAbs_Edge; }
};
#endif
