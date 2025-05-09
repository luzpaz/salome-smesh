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

//  File   : StdMeshers_QuadToTriaAdaptor.hxx
//  Module : SMESH
//
#ifndef _SMESH_QuadToTriaAdaptor_HXX_
#define _SMESH_QuadToTriaAdaptor_HXX_

#include "SMESH_StdMeshers.hxx"

#include "SMESH_ProxyMesh.hxx"

#include <TColgp_Array1OfPnt.hxx>
#include <TColgp_Array1OfVec.hxx>

class SMESH_Mesh;
class SMESH_ElementSearcher;
class SMDS_MeshElement;
class SMDS_MeshNode;
class SMDS_MeshFace;
class gp_Pnt;
class gp_Vec;


#include <set>
#include <list>
#include <vector>

#include <TopoDS_Shape.hxx>
#include <NCollection_DataMap.hxx>

/*!
 * \brief "Transforms" quadrilateral faces into triangular ones by creation of pyramids
 */
class STDMESHERS_EXPORT StdMeshers_QuadToTriaAdaptor : public SMESH_ProxyMesh
{
public:
  StdMeshers_QuadToTriaAdaptor();

  ~StdMeshers_QuadToTriaAdaptor();

  bool Compute(SMESH_Mesh&         aMesh,
               const TopoDS_Shape& aShape,
               SMESH_ProxyMesh*    aProxyMesh=0);

  bool Compute(SMESH_Mesh& aMesh);

  const TopoDS_Shape& GetShape() const { return myShape; }

protected:

  int Preparation(const SMDS_MeshElement* face,
                  TColgp_Array1OfPnt& PN,
                  TColgp_Array1OfVec& VN,
                  std::vector<const SMDS_MeshNode*>& FNodes,
                  gp_Pnt& PC, gp_Vec& VNorm,
                  const SMDS_MeshElement** volumes=0);

  bool LimitHeight (gp_Pnt&                                  Papex,
                    const gp_Pnt&                            PC,
                    const TColgp_Array1OfPnt&                PN,
                    const std::vector<const SMDS_MeshNode*>& FNodes,
                    SMESH_Mesh&                              aMesh,
                    const SMDS_MeshElement*                  NotCheckedFace,
                    const bool                               UseApexRay,
                    const TopoDS_Shape&                      Shape = TopoDS_Shape());

  bool Compute2ndPart(SMESH_Mesh&                                 aMesh,
                      const std::vector<const SMDS_MeshElement*>& pyramids);


  void MergePiramids( const SMDS_MeshElement*          PrmI,
                      const SMDS_MeshElement*          PrmJ,
                      std::set<const SMDS_MeshNode*> & nodesToMove);

  void MergeAdjacent(const SMDS_MeshElement*         PrmI,
                     std::set<const SMDS_MeshNode*>& nodesToMove,
                     const bool                      isRecursion = false);

  bool DecreaseHeightDifference( const SMDS_MeshElement* pyram,
                                 const double            h2 );

  TopoDS_Shape                      myShape;
  std::set<const SMDS_MeshElement*> myRemovedTrias;
  std::list< const SMDS_MeshNode* > myDegNodes;
  const SMESH_ElementSearcher*      myElemSearcher;

  NCollection_DataMap< const SMDS_MeshElement*, double > myPyramHeight2;

  // work buffers of DecreaseHeightDifference()
  std::vector< const SMDS_MeshElement* > myAdjPyrams;
  std::vector<const SMDS_MeshNode *>     myNodes;
};

#endif
