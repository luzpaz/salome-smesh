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

// File:    SMDS_UnstructuredGrid.hxx
// Author:  prascle
// Created: September 16, 2009, 10:28 PM

#ifndef _SMDS_UNSTRUCTUREDGRID_HXX
#define _SMDS_UNSTRUCTUREDGRID_HXX

#include "SMESH_SMDS.hxx"

#include <vtkUnstructuredGrid.h>
#include <vtkCellLinks.h>
#include <smIdType.hxx>

#include <vector>
#include <set>
#include <map>

//#define VTK_HAVE_POLYHEDRON
//#ifdef VTK_HAVE_POLYHEDRON
#define VTK_MAXTYPE VTK_POLYHEDRON
//#else
//  #define VTK_MAXTYPE VTK_QUADRATIC_PYRAMID
//#endif

#define NBMAXNEIGHBORS 100

// allow very huge polyhedrons in tests
#define NBMAXNODESINCELL 5000

// Keep compatibility with paraview 5.0.1 on Linux
#ifndef WIN32
  #ifndef VTK_HAS_MTIME_TYPE
  #define VTK_HAS_MTIME_TYPE
  typedef unsigned long int vtkMTimeType;
  #endif
#endif

class SMDS_Downward;
class SMDS_Mesh;
class SMDS_MeshCell;
class SMDS_MeshVolume;

class SMDS_EXPORT SMDS_CellLinks: public vtkCellLinks
{
public:
  void ResizeForPoint(vtkIdType vtkID);
  void BuildLinks(vtkDataSet *data, vtkCellArray *Connectivity, vtkUnsignedCharArray* types);
  static SMDS_CellLinks* New();
protected:
  SMDS_CellLinks();
  ~SMDS_CellLinks();
};

class SMDS_EXPORT SMDS_UnstructuredGrid: public vtkUnstructuredGrid
{
public:
  void setSMDS_mesh(SMDS_Mesh *mesh);
  void compactGrid(std::vector<smIdType>& idNodesOldToNew,
                   smIdType               newNodeSize,
                   std::vector<smIdType>& idCellsOldToNew,
                   smIdType               newCellSize);
  virtual vtkMTimeType GetMTime();
  virtual vtkPoints *GetPoints();

  vtkIdType InsertNextLinkedCell(int type, int npts, vtkIdType *pts);

  int CellIdToDownId(vtkIdType vtkCellId);
  void setCellIdToDownId(vtkIdType vtkCellId, int downId);
  void CleanDownwardConnectivity();
  void BuildDownwardConnectivity(bool withEdges);
  int GetNeighbors(int* neighborsVtkIds, int* downIds, unsigned char* downTypes, int vtkId, bool getSkin=false);
  int GetParentVolumes(int* volVtkIds, int vtkId);
  int GetParentVolumes(int* volVtkIds, int downId, unsigned char downType);
  void GetNodeIds(std::set<int>& nodeSet, int downId, unsigned char downType);
  void ModifyCellNodes(int vtkVolId, std::map<int, int> localClonedNodeIds);
  int getOrderedNodesOfFace(int vtkVolId, int& dim, std::vector<vtkIdType>& orderedNodes);
  SMDS_MeshCell* extrudeVolumeFromFace(int vtkVolId, int domain1, int domain2,
                                       std::set<int>&                      originalNodes,
                                       std::map<int, std::map<int, int> >& nodeDomains,
                                       std::map<int, std::map<long,int> >& nodeQuadDomains);
  void BuildLinks();
  void DeleteLinks();
  SMDS_CellLinks* GetLinks();
  bool HasLinks() const { return this->Links; }

  SMDS_Downward* getDownArray(unsigned char vtkType)
  {
    return _downArray[vtkType];
  }
  void AllocateDiameters( vtkIdType maxVtkID );
  void SetBallDiameter( vtkIdType vtkID, double diameter );
  double GetBallDiameter( vtkIdType vtkID ) const;

  static SMDS_UnstructuredGrid* New();
  SMDS_Mesh *_mesh;

protected:
  SMDS_UnstructuredGrid();
  ~SMDS_UnstructuredGrid();
  void copyNodes(vtkPoints *newPoints, std::vector<smIdType>& idNodesOldToNew, vtkIdType& alreadyCopied, vtkIdType start, vtkIdType end);
  void copyBloc(vtkUnsignedCharArray *newTypes,
                const std::vector<smIdType>& idCellsOldToNew,
                const std::vector<smIdType>& idNodesOldToNew,
                vtkCellArray* newConnectivity,
                vtkIdTypeArray* newLocations,
                std::vector<vtkIdType>& pointsCell);

  std::vector<int> _cellIdToDownId; //!< convert vtk Id to downward[vtkType] id, initialized with -1
  std::vector<unsigned char> _downTypes;
  std::vector<SMDS_Downward*> _downArray;
};

#endif  /* _SMDS_UNSTRUCTUREDGRID_HXX */

