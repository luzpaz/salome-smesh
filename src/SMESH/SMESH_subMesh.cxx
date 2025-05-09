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

//  SMESH SMESH : implementation of SMESH idl descriptions
//  File   : SMESH_subMesh.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#include "SMESH_subMesh.hxx"

#include "SMDS_SetIterator.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Algo.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Hypothesis.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_MesherHelper.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "SMESH_MeshLocker.hxx"

#include "utilities.h"
#include "Basics_Utils.hxx"

#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_ListIteratorOfListOfShape.hxx>
#include <TopTools_ListOfShape.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopoDS_Iterator.hxx>
#include <gp_Pnt.hxx>

#include <Standard_OutOfMemory.hxx>
#include <Standard_ErrorHandler.hxx>

#include <numeric>

using namespace std;

#ifdef _DEBUG_
// enable printing algo + shape id + hypo used while meshing
#define PRINT_WHO_COMPUTE_WHAT
#endif

//=============================================================================
/*!
 * \brief Allocate some memory at construction and release it at destruction.
 * Is used to be able to continue working after mesh generation breaks due to
 * lack of memory
 */
//=============================================================================

struct MemoryReserve
{
  char* myBuf;
  MemoryReserve(): myBuf( new char[1024*1024*2] ){}
  ~MemoryReserve() { delete [] myBuf; }
};

//=============================================================================
/*!
 *  default constructor:
 */
//=============================================================================

SMESH_subMesh::SMESH_subMesh(int                  Id,
                             SMESH_Mesh *         father,
                             SMESHDS_Mesh *       meshDS,
                             const TopoDS_Shape & aSubShape)
{
  _subShape           = aSubShape;
  _subMeshDS          = meshDS->MeshElements(_subShape);   // may be null ...
  _father             = father;
  _Id                 = Id;
  _dependenceAnalysed = _alwaysComputed = false;
  _algo               = 0;
  if (_subShape.ShapeType() == TopAbs_VERTEX)
  {
    _algoState = HYP_OK;
    _computeState = READY_TO_COMPUTE;
  }
  else
  {
    _algoState = NO_ALGO;
    _computeState = NOT_READY;
  }
  _computeCost = 0; // how costly is to compute this sub-mesh
  _realComputeCost = 0;
  _allowedSubShapes = nullptr;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESH_subMesh::~SMESH_subMesh()
{
  deleteOwnListeners();
}

//=============================================================================
/*!
 *
 */
//=============================================================================

int SMESH_subMesh::GetId() const
{
  //MESSAGE("SMESH_subMesh::GetId");
  return _Id;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESHDS_SubMesh * SMESH_subMesh::GetSubMeshDS()
{
  // submesh appears in DS only when a mesher set nodes and elements on a shape
  return _subMeshDS ? _subMeshDS : _subMeshDS = _father->GetMeshDS()->MeshElements(_subShape); // may be null
}

//=============================================================================
/*!
 *
 */
//=============================================================================

const SMESHDS_SubMesh * SMESH_subMesh::GetSubMeshDS() const
{
  return ((SMESH_subMesh*) this )->GetSubMeshDS();
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESHDS_SubMesh* SMESH_subMesh::CreateSubMeshDS()
{
  if ( !GetSubMeshDS() ) {
    SMESHDS_Mesh* meshDS = _father->GetMeshDS();
    meshDS->NewSubMesh( meshDS->ShapeToIndex( _subShape ) );
  }
  return GetSubMeshDS();
}

//=============================================================================
/*!
 *
 */
//=============================================================================

SMESH_subMesh *SMESH_subMesh::GetFirstToCompute()
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(true,false);
  while ( smIt->more() ) {
    SMESH_subMesh *sm = smIt->next();
    if ( sm->GetComputeState() == READY_TO_COMPUTE )
      return sm;
  }
  return 0;                     // nothing to compute
}

//================================================================================
/*!
 * \brief Returns a current algorithm
 */
//================================================================================

SMESH_Algo* SMESH_subMesh::GetAlgo() const
{
  if ( !_algo )
  {
    SMESH_subMesh* me = const_cast< SMESH_subMesh* >( this );
    me->_algo = _father->GetGen()->GetAlgo( me, & me->_algoShape );
  }
  return _algo;
}

//================================================================================
/*!
 * \brief Allow algo->Compute() if a sub-shape of lower dim is meshed but
 *        none mesh entity is bound to it (PAL13615, 2nd part)
 */
//================================================================================

void SMESH_subMesh::SetIsAlwaysComputed(bool isAlCo)
{
  _alwaysComputed = isAlCo;
  if ( _alwaysComputed )
    _computeState = COMPUTE_OK;
  else
    ComputeStateEngine( CHECK_COMPUTE_STATE );
}

//=======================================================================
/*!
 * \brief Return true if no mesh entities is bound to the submesh
 */
//=======================================================================

bool SMESH_subMesh::IsEmpty() const
{
  if (SMESHDS_SubMesh * subMeshDS = ((SMESH_subMesh*)this)->GetSubMeshDS())
    return (!subMeshDS->NbElements() && !subMeshDS->NbNodes());
  return true;
}

//=======================================================================
//function : IsMeshComputed
//purpose  : check if _subMeshDS contains mesh elements
//=======================================================================

bool SMESH_subMesh::IsMeshComputed() const
{
  if ( _alwaysComputed )
    return true;
  // algo may bind a sub-mesh not to _subShape, eg 3D algo
  // sets nodes on SHELL while _subShape may be SOLID

  SMESHDS_Mesh* meshDS = _father->GetMeshDS();
  int dim = SMESH_Gen::GetShapeDim( _subShape );
  int type = _subShape.ShapeType();
  for ( ; type <= TopAbs_VERTEX; type++) {
    if ( dim == SMESH_Gen::GetShapeDim( (TopAbs_ShapeEnum) type ))
    {
      TopExp_Explorer exp( _subShape, (TopAbs_ShapeEnum) type );
      for ( ; exp.More(); exp.Next() )
      {
        if ( SMESHDS_SubMesh * smDS = meshDS->MeshElements( exp.Current() ) )
        {
          bool computed = (dim > 0) ? smDS->NbElements() : smDS->NbNodes();
          if ( computed )
            return true;
        }
      }
    }
    else
      break;
  }

  return false;
}

//================================================================================
/*!
 * \brief Check if any upper level sub-shape is not computed.
 *        Used to update a sub-mesh icon
 */
//================================================================================

bool SMESH_subMesh::IsComputedPartially() const
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(/*includeSelf=*/true,
                                                       /*SolidFirst=*/true);
  bool allComputed = true;
  TopAbs_ShapeEnum readyType = TopAbs_VERTEX; // max value
  while ( smIt->more() && allComputed )
  {
    SMESH_subMesh* sm = smIt->next();

    if ( sm->GetSubShape().ShapeType() > readyType )
      break; // lower dimension -> stop
    if ( sm->GetComputeState() != SMESH_subMesh::NOT_READY )
      readyType = sm->GetSubShape().ShapeType();

    switch ( sm->GetComputeState() )
    {
    case SMESH_subMesh::READY_TO_COMPUTE:
    case SMESH_subMesh::FAILED_TO_COMPUTE:
      allComputed = false;// sm->IsMeshComputed();
      break;
    case SMESH_subMesh::NOT_READY:
    case SMESH_subMesh::COMPUTE_OK:
      continue;
    }
  }
  return !allComputed;
}

//=============================================================================
/*!
 * Return true if all sub-meshes have been meshed
 */
//=============================================================================

bool SMESH_subMesh::SubMeshesComputed(bool * isFailedToCompute/*=0*/) const
{
  int myDim = SMESH_Gen::GetShapeDim( _subShape );
  int dimToCheck = myDim - 1;
  bool subMeshesComputed = true;
  if ( isFailedToCompute ) *isFailedToCompute = false;
  // check sub-meshes with upper dimension => reverse iteration
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,true);
  while ( smIt->more() )
  {
    SMESH_subMesh *sm = smIt->next();
    if ( sm->_alwaysComputed )
      continue;
    const TopoDS_Shape & ss = sm->GetSubShape();

    // MSV 07.04.2006: restrict checking to myDim-1 only. Ex., there is no sense
    // in checking of existence of edges if the algo needs only faces. Moreover,
    // degenerated edges may have no sub-mesh, as after computing NETGEN_2D.
    if ( !_algo || _algo->NeedDiscreteBoundary() ) {
      int dim = SMESH_Gen::GetShapeDim( ss );
      if (dim < dimToCheck)
        break; // the rest sub-meshes are all of less dimension
    }
    SMESHDS_SubMesh * ds = sm->GetSubMeshDS();
    bool computeOk = ((sm->GetComputeState() == COMPUTE_OK ) ||
                      (ds && ( dimToCheck ? ds->NbElements() : ds->NbNodes() )));
    if (!computeOk)
    {
      subMeshesComputed = false;

      if ( isFailedToCompute && !(*isFailedToCompute) )
        *isFailedToCompute = ( sm->GetComputeState() == FAILED_TO_COMPUTE );

      if ( !isFailedToCompute )
        break;
    }
  }
  return subMeshesComputed;
}

//================================================================================
/*!
 * \brief Return cost of computing this sub-mesh. If hypotheses are not well defined,
 *        zero is returned
 *  \return int - the computation cost in abstract units.
 */
//================================================================================

int SMESH_subMesh::GetComputeCost() const
{
  return _realComputeCost;
}

//================================================================================
/*!
 * \brief Return cost of computing this sub-mesh. The cost depends on the shape type
 *        and number of sub-meshes this one DependsOn().
 *  \return int - the computation cost in abstract units.
 */
//================================================================================

int SMESH_subMesh::computeCost() const
{
  if ( !_computeCost )
  {
    int computeCost;
    switch ( _subShape.ShapeType() ) {
    case TopAbs_SOLID:
    case TopAbs_SHELL: computeCost = 5000; break;
    case TopAbs_FACE:  computeCost = 500; break;
    case TopAbs_EDGE:  computeCost = 2; break;
    default:           computeCost = 1;
    }
    SMESH_subMeshIteratorPtr childIt = getDependsOnIterator(/*includeSelf=*/false);
    while ( childIt->more() )
      computeCost += childIt->next()->computeCost();

    ((SMESH_subMesh*)this)->_computeCost = computeCost;
  }
  return _computeCost;
}

//=============================================================================
/*!
 * Returns all sub-meshes this one depend on
 */
//=============================================================================

const std::map < int, SMESH_subMesh * >& SMESH_subMesh::DependsOn()
{
  if ( _dependenceAnalysed || !_father->HasShapeToMesh() )
    return _mapDepend;

  int type = _subShape.ShapeType();
  switch (type)
  {
  case TopAbs_COMPOUND:
  {
    list< TopoDS_Shape > compounds( 1, _subShape );
    list< TopoDS_Shape >::iterator comp = compounds.begin();
    for ( ; comp != compounds.end(); ++comp )
    {
      for ( TopoDS_Iterator sub( *comp ); sub.More(); sub.Next() )
        switch ( sub.Value().ShapeType() )
        {
        case TopAbs_COMPOUND:  compounds.push_back( sub.Value() ); break;
        case TopAbs_COMPSOLID: insertDependence( sub.Value(), TopAbs_SOLID ); break;
        case TopAbs_SOLID:     insertDependence( sub.Value(), TopAbs_SOLID ); break;
        case TopAbs_SHELL:     insertDependence( sub.Value(), TopAbs_FACE ); break;
        case TopAbs_FACE:      insertDependence( sub.Value(), TopAbs_FACE ); break;
        case TopAbs_WIRE:      insertDependence( sub.Value(), TopAbs_EDGE ); break;
        case TopAbs_EDGE:      insertDependence( sub.Value(), TopAbs_EDGE ); break;
        case TopAbs_VERTEX:    insertDependence( sub.Value(), TopAbs_VERTEX ); break;
        default:;
        }
    }
  }
  break;
  case TopAbs_COMPSOLID: insertDependence( _subShape, TopAbs_SOLID ); break;
  case TopAbs_SOLID:     insertDependence( _subShape, TopAbs_FACE );
  { /*internal EDGE*/    insertDependence( _subShape, TopAbs_EDGE, TopAbs_WIRE ); break; }
  case TopAbs_SHELL:     insertDependence( _subShape, TopAbs_FACE ); break;
  case TopAbs_FACE:      insertDependence( _subShape, TopAbs_EDGE ); break;
  case TopAbs_WIRE:      insertDependence( _subShape, TopAbs_EDGE ); break;
  case TopAbs_EDGE:      insertDependence( _subShape, TopAbs_VERTEX ); break;
  default:;
  }
  _dependenceAnalysed = true;
  return _mapDepend;
}

//================================================================================
/*!
 * \brief Return a key for SMESH_subMesh::_mapDepend map
 */
//================================================================================

namespace
{
  int dependsOnMapKey( TopAbs_ShapeEnum type, int shapeID )
  {
    int ordType = 9 - int(type);               // 2 = Vertex, 8 = CompSolid
    int     cle = shapeID;
    cle += 10000000 * ordType;    // sort map by ordType then index
    return cle;
  }
  int dependsOnMapKey( const SMESH_subMesh* sm )
  {
    return dependsOnMapKey( sm->GetSubShape().ShapeType(), sm->GetId() );
  }
}

//=============================================================================
/*!
 * Add sub-meshes on sub-shapes of a given type into the dependence map.
 */
//=============================================================================

void SMESH_subMesh::insertDependence(const TopoDS_Shape aShape,
                                     TopAbs_ShapeEnum   aSubType,
                                     TopAbs_ShapeEnum   avoidType)
{
  TopExp_Explorer sub( aShape, aSubType, avoidType );
  for ( ; sub.More(); sub.Next() )
  {
    SMESH_subMesh *aSubMesh = _father->GetSubMesh( sub.Current() );
    if ( aSubMesh->GetId() == 0 )
      continue;  // not a sub-shape of the shape to mesh
    int cle = dependsOnMapKey( aSubMesh );
    if ( _mapDepend.find( cle ) == _mapDepend.end())
    {
      _mapDepend[cle] = aSubMesh;
      const map < int, SMESH_subMesh * > & subMap = aSubMesh->DependsOn();
      _mapDepend.insert( subMap.begin(), subMap.end() );
    }
  }
}

//================================================================================
/*!
 * \brief Return \c true if \a this sub-mesh depends on \a other
 */
//================================================================================

bool SMESH_subMesh::DependsOn( const SMESH_subMesh* other ) const
{
  return other ? _mapDepend.count( dependsOnMapKey( other )) : false;
}

//================================================================================
/*!
 * \brief Return \c true if \a this sub-mesh depends on a \a shape
 */
//================================================================================

bool SMESH_subMesh::DependsOn( const int shapeID ) const
{
  return DependsOn( _father->GetSubMeshContaining( shapeID ));
}

//=============================================================================
/*!
 * Return a shape of \a this sub-mesh
 */
//=============================================================================

const TopoDS_Shape & SMESH_subMesh::GetSubShape() const
{
  return _subShape;
}

//=======================================================================
//function : CanAddHypothesis
//purpose  : return true if theHypothesis can be attached to me:
//           its dimension is checked
//=======================================================================

bool SMESH_subMesh::CanAddHypothesis(const SMESH_Hypothesis* theHypothesis) const
{
  int aHypDim   = theHypothesis->GetDim();
  int aShapeDim = SMESH_Gen::GetShapeDim(_subShape);
  // issue 21106. Forbid 3D mesh on the SHELL
  // if (aHypDim == 3 && aShapeDim == 3) {
  //   // check case of open shell
  //   //if (_subShape.ShapeType() == TopAbs_SHELL && !_subShape.Closed())
  //   if (_subShape.ShapeType() == TopAbs_SHELL && !BRep_Tool::IsClosed(_subShape))
  //     return false;
  // }
  if ( aHypDim <= aShapeDim )
    return true;

  return false;
}

//=======================================================================
//function : IsApplicableHypothesis
//purpose  : check if this sub-mesh can be computed using a hypothesis
//=======================================================================

bool SMESH_subMesh::IsApplicableHypothesis(const SMESH_Hypothesis* theHypothesis) const
{
  if ( !_father->HasShapeToMesh() && _subShape.ShapeType() == TopAbs_SOLID )
    return true; // true for the PseudoShape

  return IsApplicableHypothesis( theHypothesis, _subShape.ShapeType() );
}

//=======================================================================
//function : IsApplicableHypothesis
//purpose  : compare shape type and hypothesis type
//=======================================================================

bool SMESH_subMesh::IsApplicableHypothesis(const SMESH_Hypothesis* theHypothesis,
                                           const TopAbs_ShapeEnum  theShapeType)
{
  if ( theHypothesis->GetType() > SMESHDS_Hypothesis::PARAM_ALGO)
  {
    // algorithm
    if ( theHypothesis->GetShapeType() & (1<< theShapeType))
      // issue 21106. Forbid 3D mesh on the SHELL
      return !( theHypothesis->GetDim() == 3 && theShapeType == TopAbs_SHELL );
    else
      return false;
  }

  // hypothesis
  switch ( theShapeType ) {
  case TopAbs_VERTEX:
  case TopAbs_EDGE:
  case TopAbs_FACE:
  case TopAbs_SOLID:
    return SMESH_Gen::GetShapeDim( theShapeType ) == theHypothesis->GetDim();

  case TopAbs_SHELL:
    // Special case for algorithms, building 2D mesh on a whole shell.
    // Before this fix there was a problem after restoring from study,
    // because in that case algorithm is assigned before hypothesis
    // (on shell in problem case) and hypothesis is checked on faces
    // (because it is 2D), where we have NO_ALGO state.
    // Now 2D hypothesis is also applicable to shells.
    return (theHypothesis->GetDim() == 2 || theHypothesis->GetDim() == 3);

//   case TopAbs_WIRE:
//   case TopAbs_COMPSOLID:
//   case TopAbs_COMPOUND:
  default:;
  }
  return false;
}

//================================================================================
/*!
 * \brief Treats modification of hypotheses definition
 *  \param [in] event - what happens
 *  \param [in] anHyp - a hypothesis
 *  \return SMESH_Hypothesis::Hypothesis_Status - a treatment result.
 *
 * Optional description of a problematic situation (if any) can be retrieved
 * via GetComputeError().
 */
//================================================================================

SMESH_Hypothesis::Hypothesis_Status
  SMESH_subMesh::AlgoStateEngine(algo_event event, SMESH_Hypothesis * anHyp)
{
  // **** les retour des evenement shape sont significatifs
  // (add ou remove fait ou non)
  // le retour des evenement father n'indiquent pas que add ou remove fait

  SMESH_Hypothesis::Hypothesis_Status aux_ret, ret = SMESH_Hypothesis::HYP_OK;
  if ( _Id == 0 ) return ret; // not a sub-shape of the shape to mesh

  SMESHDS_Mesh* meshDS =_father->GetMeshDS();
  SMESH_Algo*   algo   = 0;
  _algo = 0;

  if (_subShape.ShapeType() == TopAbs_VERTEX )
  {
    if ( anHyp->GetDim() != 0) {
      if (event == ADD_HYP || event == ADD_ALGO)
        return SMESH_Hypothesis::HYP_BAD_DIM;
      else
        return SMESH_Hypothesis::HYP_OK;
    }
    // 0D hypothesis
    else if ( _algoState == HYP_OK ) {
      // update default _algoState
      if ( event != REMOVE_FATHER_ALGO )
      {
        _algoState = NO_ALGO;
        algo = GetAlgo();
        if ( algo ) {
          _algoState = MISSING_HYP;
          if ( event == REMOVE_FATHER_HYP ||
               algo->CheckHypothesis(*_father,_subShape, aux_ret))
            _algoState = HYP_OK;
        }
      }
    }
  }

  int oldAlgoState = _algoState;
  bool modifiedHyp = (event == MODIF_HYP);  // if set to true, force event MODIF_ALGO_STATE
  SMESH_Algo* algoRequiringCleaning = 0;

  bool isApplicableHyp = IsApplicableHypothesis( anHyp );

  if (event == ADD_ALGO || event == ADD_FATHER_ALGO)
  {
    // -------------------------------------------
    // check if a shape needed by algo is present
    // -------------------------------------------
    algo = static_cast< SMESH_Algo* >( anHyp );
    if ( !_father->HasShapeToMesh() && algo->NeedShape() )
      return SMESH_Hypothesis::HYP_NEED_SHAPE;
    // ----------------------
    // check mesh conformity
    // ----------------------
    if (isApplicableHyp && !_father->IsNotConformAllowed() && !IsConform( algo ))
      return SMESH_Hypothesis::HYP_NOTCONFORM;

    // check if all-dimensional algo is hidden by other local one
    if ( event == ADD_ALGO ) {
      SMESH_HypoFilter filter( SMESH_HypoFilter::HasType( algo->GetType() ));
      filter.Or( SMESH_HypoFilter::HasType( algo->GetType()+1 ));
      filter.Or( SMESH_HypoFilter::HasType( algo->GetType()+2 ));
      if ( SMESH_Algo * curAlgo = (SMESH_Algo*)_father->GetHypothesis( this, filter, true ))
        if ( !curAlgo->NeedDiscreteBoundary() && curAlgo != anHyp )
          algoRequiringCleaning = curAlgo;
    }
  }

  // ----------------------------------
  // add a hypothesis to DS if possible
  // ----------------------------------
  if (event == ADD_HYP || event == ADD_ALGO)
  {
    if ( ! CanAddHypothesis( anHyp )) // check dimension
      return SMESH_Hypothesis::HYP_BAD_DIM;

    if ( !anHyp->IsAuxiliary() && getSimilarAttached( _subShape, anHyp ) )
      return SMESH_Hypothesis::HYP_ALREADY_EXIST;

    if ( !meshDS->AddHypothesis(_subShape, anHyp))
      return SMESH_Hypothesis::HYP_ALREADY_EXIST;
  }

  // --------------------------
  // remove a hypothesis from DS
  // --------------------------
  if (event == REMOVE_HYP || event == REMOVE_ALGO)
  {
    if (!meshDS->RemoveHypothesis(_subShape, anHyp))
      return SMESH_Hypothesis::HYP_OK; // nothing changes

    if (event == REMOVE_ALGO)
    {
      algo = dynamic_cast<SMESH_Algo*> (anHyp);
      if (!algo->NeedDiscreteBoundary())
        algoRequiringCleaning = algo;
    }
  }

  // ------------------
  // analyse algo state
  // ------------------
  if (!isApplicableHyp)
    return ret; // not applicable hypotheses do not change algo state

  if (( algo = GetAlgo()))
    algo->InitComputeError();

  switch (_algoState)
  {

    // ----------------------------------------------------------------------

  case NO_ALGO:
    switch (event) {
    case ADD_HYP:
      break;
    case ADD_ALGO: {
      algo = GetAlgo();
      ASSERT(algo);
      if (algo->CheckHypothesis((*_father),_subShape, aux_ret))
        setAlgoState(HYP_OK);
      else if ( algo->IsStatusFatal( aux_ret )) {
        meshDS->RemoveHypothesis(_subShape, anHyp);
        ret = aux_ret;
      }
      else
        setAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_HYP:
    case REMOVE_ALGO:
    case ADD_FATHER_HYP:
      break;
    case ADD_FATHER_ALGO: {    // Algo just added in father
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo == anHyp ) {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret))
          setAlgoState(HYP_OK);
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    case REMOVE_FATHER_HYP:
      break;
    case REMOVE_FATHER_ALGO: {
      algo = GetAlgo();
      if (algo)
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
            setAlgoState(HYP_OK);
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    case MODIF_HYP: break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case MISSING_HYP:
    switch (event)
    {
    case ADD_HYP: {
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, ret ))
        setAlgoState(HYP_OK);
      if (SMESH_Hypothesis::IsStatusFatal( ret ))
        meshDS->RemoveHypothesis(_subShape, anHyp);
      else if (!_father->IsUsedHypothesis( anHyp, this ))
      {
        meshDS->RemoveHypothesis(_subShape, anHyp);
        ret = SMESH_Hypothesis::HYP_INCOMPATIBLE;
      }
      break;
    }
    case ADD_ALGO: {           //already existing algo : on father ?
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))// ignore hyp status
        setAlgoState(HYP_OK);
      else if ( algo->IsStatusFatal( aux_ret )) {
        meshDS->RemoveHypothesis(_subShape, anHyp);
        ret = aux_ret;
      }
      else
        setAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_HYP:
      break;
    case REMOVE_ALGO: {        // perhaps a father algo applies ?
      algo = GetAlgo();
      if (algo == NULL)  // no more algo applying on sub-shape...
      {
        setAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
          setAlgoState(HYP_OK);
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    case MODIF_HYP: // assigned hypothesis value may become good
    case ADD_FATHER_HYP: {
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
        setAlgoState(HYP_OK);
      else
        setAlgoState(MISSING_HYP);
      break;
    }
    case ADD_FATHER_ALGO: { // new father algo
      algo = GetAlgo();
      ASSERT( algo );
      if ( algo == anHyp ) {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
          setAlgoState(HYP_OK);
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    case REMOVE_FATHER_HYP:    // nothing to do
      break;
    case REMOVE_FATHER_ALGO: {
      algo = GetAlgo();
      if (algo == NULL)  // no more applying algo on father
      {
        setAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape , aux_ret ))
          setAlgoState(HYP_OK);
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case HYP_OK:
    switch (event)
    {
    case ADD_HYP: {
      algo = GetAlgo();
      ASSERT(algo);
      if (!algo->CheckHypothesis((*_father),_subShape, ret ))
      {
        if ( !SMESH_Hypothesis::IsStatusFatal( ret ))
          // ret should be fatal: anHyp was not added
          ret = SMESH_Hypothesis::HYP_INCOMPATIBLE;
      }
      else if (!_father->IsUsedHypothesis( anHyp, this ))
      {
        if ( anHyp->GetDim() == this->GetAlgo()->GetDim() )
          ret = SMESH_Hypothesis::HYP_INCOMPATIBLE;
      }

      if (SMESH_Hypothesis::IsStatusFatal( ret ))
      {
        MESSAGE("do not add extra hypothesis");
        meshDS->RemoveHypothesis(_subShape, anHyp);
      }
      else
      {
        modifiedHyp = true;
      }
      break;
    }
    case ADD_ALGO: {           //already existing algo : on father ?
      algo = GetAlgo();
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
        // check if algo changes
        SMESH_HypoFilter f;
        f.Init(   SMESH_HypoFilter::IsAlgo() );
        f.And(    SMESH_HypoFilter::IsApplicableTo( _subShape ));
        f.AndNot( SMESH_HypoFilter::Is( algo ));
        const SMESH_Hypothesis * prevAlgo = _father->GetHypothesis( this, f, true );
        if (prevAlgo &&
            string( algo->GetName()) != prevAlgo->GetName())
        {
          oldAlgoState = NO_ALGO; // force setting event listener (#16648)
          modifiedHyp  = true;
        }
      }
      else
        setAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_HYP: {
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
        setAlgoState(HYP_OK);
      else
        setAlgoState(MISSING_HYP);
      modifiedHyp = true;
      break;
    }
    case REMOVE_ALGO: {         // perhaps a father algo applies ?
      algo = GetAlgo();
      if (algo == NULL)   // no more algo applying on sub-shape...
      {
        setAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
          // check if algo remains
          if ( anHyp != algo && strcmp( anHyp->GetName(), algo->GetName()) )
            modifiedHyp = true;
        }
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    case MODIF_HYP: // hypothesis value may become bad
    case ADD_FATHER_HYP: {  // new father hypothesis ?
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret ))
      {
        if (_father->IsUsedHypothesis( anHyp, this )) // new Hyp
          modifiedHyp = true;
      }
      else
        setAlgoState(MISSING_HYP);
      break;
    }
    case ADD_FATHER_ALGO: {
      algo = GetAlgo();
      if ( algo == anHyp ) { // a new algo on father
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
          // check if algo changes
          SMESH_HypoFilter f;
          f.Init(   SMESH_HypoFilter::IsAlgo() );
          f.And(    SMESH_HypoFilter::IsApplicableTo( _subShape ));
          f.AndNot( SMESH_HypoFilter::Is( algo ));
          const SMESH_Hypothesis* prevAlgo = _father->GetHypothesis( this, f, true );
          if (prevAlgo &&
              string(algo->GetName()) != string(prevAlgo->GetName()) )
            modifiedHyp = true;
        }
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    case REMOVE_FATHER_HYP: {
      algo = GetAlgo();
      ASSERT(algo);
      if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
        // is there the same local hyp or maybe a new father algo applied?
        if ( !getSimilarAttached( _subShape, anHyp ) )
          modifiedHyp = true;
      }
      else
        setAlgoState(MISSING_HYP);
      break;
    }
    case REMOVE_FATHER_ALGO: {
      // IPAL21346. Edges not removed when Netgen 1d-2d is removed from a SOLID.
      // CLEAN was not called at event REMOVE_ALGO because the algo is not applicable to SOLID.
      algo = dynamic_cast<SMESH_Algo*> (anHyp);
      if (!algo->NeedDiscreteBoundary())
        algoRequiringCleaning = algo;
      algo = GetAlgo();
      if (algo == NULL)  // no more applying algo on father
      {
        setAlgoState(NO_ALGO);
      }
      else
      {
        if ( algo->CheckHypothesis((*_father),_subShape, aux_ret )) {
          // check if algo changes
          if ( string(algo->GetName()) != string( anHyp->GetName()) )
            modifiedHyp = true;
        }
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    }
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  default:
    ASSERT(0);
    break;
  }

  // detect algorithm hiding
  //
  if ( ret == SMESH_Hypothesis::HYP_OK &&
       ( event == ADD_ALGO || event == ADD_FATHER_ALGO ) && algo &&
       algo->GetName() == anHyp->GetName() )
  {
    // is algo hidden?
    SMESH_Gen* gen = _father->GetGen();
    const std::vector< SMESH_subMesh * > & ancestors = GetAncestors();
    for ( size_t iA = 0; ( ret == SMESH_Hypothesis::HYP_OK && iA < ancestors.size()); ++iA ) {
      if ( SMESH_Algo* upperAlgo = ancestors[ iA ]->GetAlgo() )
        if ( !upperAlgo->NeedDiscreteBoundary() && !upperAlgo->SupportSubmeshes())
          ret = SMESH_Hypothesis::HYP_HIDDEN_ALGO;
    }
    // is algo hiding?
    if ( ret == SMESH_Hypothesis::HYP_OK &&
         !algo->NeedDiscreteBoundary()    &&
         !algo->SupportSubmeshes())
    {
      TopoDS_Shape algoAssignedTo, otherAssignedTo;
      gen->GetAlgo( this, &algoAssignedTo );
      map<int, SMESH_subMesh*>::reverse_iterator i_sm = _mapDepend.rbegin();
      for ( ; ( ret == SMESH_Hypothesis::HYP_OK && i_sm != _mapDepend.rend()) ; ++i_sm )
        if ( gen->GetAlgo( i_sm->second, &otherAssignedTo ) &&
             SMESH_MesherHelper::IsSubShape( /*sub=*/otherAssignedTo, /*main=*/algoAssignedTo ))
          ret = SMESH_Hypothesis::HYP_HIDING_ALGO;
    }
  }

  if ( _algo ) { // get an error description set by _algo->CheckHypothesis()
    _computeError = _algo->GetComputeError();
    _algo->InitComputeError();
  }

  bool stateChange = ( _algoState != oldAlgoState );

  if ( stateChange && _algoState == HYP_OK ) // hyp becomes OK
    algo->SetEventListener( this );

  if ( event == REMOVE_ALGO || event == REMOVE_FATHER_ALGO )
    _algo = 0;

  notifyListenersOnEvent( event, ALGO_EVENT, anHyp );

  if ( stateChange && oldAlgoState == HYP_OK ) { // hyp becomes KO
    deleteOwnListeners();
    SetIsAlwaysComputed( false );
    if (_subShape.ShapeType() == TopAbs_VERTEX ) {
      // restore default states
      _algoState = HYP_OK;
      _computeState = READY_TO_COMPUTE;
    }
  }

  if ( algoRequiringCleaning ) {
    // added or removed algo is all-dimensional
    ComputeStateEngine( CLEAN );
    cleanDependsOn( algoRequiringCleaning );
    ComputeSubMeshStateEngine( CHECK_COMPUTE_STATE );
  }

  if ( stateChange || modifiedHyp )
    ComputeStateEngine( MODIF_ALGO_STATE );

  _realComputeCost = ( _algoState == HYP_OK ) ? computeCost() : 0;

  return ret;
}

//=======================================================================
//function : IsConform
//purpose  : check if a conform mesh will be produced by the Algo
//=======================================================================

bool SMESH_subMesh::IsConform(const SMESH_Algo* theAlgo)
{
//  MESSAGE( "SMESH_subMesh::IsConform" );
  if ( !theAlgo ) return false;

  // Suppose that theAlgo is applicable to _subShape, do not check it here
  //if ( !IsApplicableHypothesis( theAlgo )) return false;

  // check only algo that doesn't NeedDiscreteBoundary(): because mesh made
  // on a sub-shape will be ignored by theAlgo
  if ( theAlgo->NeedDiscreteBoundary() ||
       !theAlgo->OnlyUnaryInput() ) // all adjacent shapes will be meshed by this algo?
    return true;

  // only local algo is to be checked
  //if ( gen->IsGlobalHypothesis( theAlgo, *_father ))
  if ( _subShape.ShapeType() == _father->GetMeshDS()->ShapeToMesh().ShapeType() )
    return true;

  // check algo attached to adjacent shapes

  // loop on one level down sub-meshes
  TopoDS_Iterator itsub( _subShape );
  for (; itsub.More(); itsub.Next())
  {
    // loop on adjacent subShapes
    const std::vector< SMESH_subMesh * > & ancestors = GetAncestors();
    for ( size_t iA = 0; iA < ancestors.size(); ++iA )
    {
      const TopoDS_Shape& adjacent = ancestors[ iA ]->GetSubShape();
      if ( _subShape.IsSame( adjacent )) continue;
      if ( adjacent.ShapeType() != _subShape.ShapeType())
        break;

      // check algo attached to smAdjacent
      SMESH_Algo * algo = ancestors[ iA ]->GetAlgo();
      if (algo &&
          !algo->NeedDiscreteBoundary() &&
          algo->OnlyUnaryInput())
        return false; // NOT CONFORM MESH WILL BE PRODUCED
    }
  }

  return true;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::setAlgoState(algo_state state)
{
  _algoState = state;
}

//================================================================================
/*!
 * \brief Send an event to sub-meshes
 *  \param [in] event - the event
 *  \param [in] anHyp - an hypothesis
 *  \param [in] exitOnFatal - to stop iteration on sub-meshes if a sub-mesh
 *              reports a fatal result
 *  \return SMESH_Hypothesis::Hypothesis_Status - the worst result
 *
 * Optional description of a problematic situation (if any) can be retrieved
 * via GetComputeError().
 */
//================================================================================

SMESH_Hypothesis::Hypothesis_Status
  SMESH_subMesh::SubMeshesAlgoStateEngine(algo_event         event,
                                          SMESH_Hypothesis * anHyp,
                                          bool               exitOnFatal)
{
  SMESH_Hypothesis::Hypothesis_Status ret = SMESH_Hypothesis::HYP_OK;
  //EAP: a wire (dim==1) should notify edges (dim==1)
  //EAP: int dim = SMESH_Gen::GetShapeDim(_subShape);
  //if (_subShape.ShapeType() < TopAbs_EDGE ) // wire,face etc
  {
    SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
    while ( smIt->more() ) {
      SMESH_subMesh* sm = smIt->next();
      SMESH_Hypothesis::Hypothesis_Status ret2 = sm->AlgoStateEngine(event, anHyp);
      if ( ret2 > ret )
      {
        ret = ret2;
        _computeError = sm->_computeError;
        sm->_computeError.reset();
        if ( exitOnFatal && SMESH_Hypothesis::IsStatusFatal( ret ))
          break;
      }
    }
  }
  return ret;
}

//================================================================================
/*!
 * \brief Remove elements from sub-meshes.
 *  \param algoRequiringCleaning - an all-dimensional algorithm whose presence
 *         causes the cleaning.
 */
//================================================================================

void SMESH_subMesh::cleanDependsOn( SMESH_Algo* algoRequiringCleaning/*=0*/ )
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,
                                                       /*complexShapeFirst=*/true);
  if ( _father->NbNodes() == 0 )
  {
    while ( smIt->more() )
      smIt->next()->ComputeStateEngine(CHECK_COMPUTE_STATE);
  }
  else if ( !algoRequiringCleaning || !algoRequiringCleaning->SupportSubmeshes() )
  {
    while ( smIt->more() )
      smIt->next()->ComputeStateEngine(CLEAN);
  }
  else if ( algoRequiringCleaning && algoRequiringCleaning->SupportSubmeshes() )
  {
    // find sub-meshes to keep elements on
    set< SMESH_subMesh* > smToKeep;
    TopAbs_ShapeEnum prevShapeType = TopAbs_SHAPE;
    bool toKeepPrevShapeType = false;
    while ( smIt->more() )
    {
      SMESH_subMesh* sm = smIt->next();
      sm->ComputeStateEngine(CHECK_COMPUTE_STATE);
      if ( !sm->IsEmpty() )
      {
        const bool sameShapeType = ( prevShapeType == sm->GetSubShape().ShapeType() );
        bool       keepSubMeshes = ( sameShapeType && toKeepPrevShapeType );
        if ( !sameShapeType )
        {
          // check if the algo allows presence of global algos of dimension the algo
          // can generate it-self;
          // always keep a node on VERTEX, as this node can be shared by segments
          // lying on EDGEs not shared by the VERTEX of sm, due to MergeNodes (PAL23068)
          int  shapeDim = SMESH_Gen::GetShapeDim( sm->GetSubShape() );
          keepSubMeshes = ( algoRequiringCleaning->NeedLowerHyps( shapeDim ) || shapeDim == 0 );
          prevShapeType = sm->GetSubShape().ShapeType();
          toKeepPrevShapeType = keepSubMeshes;
        }
        if ( !keepSubMeshes )
        {
          // look for a local algo used to mesh sm
          TopoDS_Shape algoShape = SMESH_MesherHelper::GetShapeOfHypothesis
            ( algoRequiringCleaning, _subShape, _father );
          SMESH_HypoFilter moreLocalAlgo;
          moreLocalAlgo.Init( SMESH_HypoFilter::IsMoreLocalThan( algoShape, *_father ));
          moreLocalAlgo.And ( SMESH_HypoFilter::IsAlgo() );
          bool localAlgoFound = _father->GetHypothesis( sm->_subShape, moreLocalAlgo, true );
          keepSubMeshes = localAlgoFound;
        }
        // remember all sub-meshes of sm
        if ( keepSubMeshes )
        {
          SMESH_subMeshIteratorPtr smIt2 = sm->getDependsOnIterator(true);
          while ( smIt2->more() )
            smToKeep.insert( smIt2->next() );
        }
      }
    }
    // remove elements
    SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,true);
    while ( smIt->more() )
    {
      SMESH_subMesh* sm = smIt->next();
      if ( !smToKeep.count( sm ))
        sm->ComputeStateEngine(CLEAN);
    }
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================

void SMESH_subMesh::DumpAlgoState(bool isMain)
{
  if (isMain)
  {
    const map < int, SMESH_subMesh * >&subMeshes = DependsOn();

    map < int, SMESH_subMesh * >::const_iterator itsub;
    for (itsub = subMeshes.begin(); itsub != subMeshes.end(); itsub++)
    {
      SMESH_subMesh *sm = (*itsub).second;
      sm->DumpAlgoState(false);
    }
  }
  MESSAGE("dim = " << SMESH_Gen::GetShapeDim(_subShape) <<
          " type of shape " << _subShape.ShapeType());
  switch (_algoState)
  {
  case NO_ALGO          : MESSAGE(" AlgoState = NO_ALGO"); break;
  case MISSING_HYP      : MESSAGE(" AlgoState = MISSING_HYP"); break;
  case HYP_OK           : MESSAGE(" AlgoState = HYP_OK");break;
  }
  switch (_computeState)
  {
  case NOT_READY        : MESSAGE(" ComputeState = NOT_READY");break;
  case READY_TO_COMPUTE : MESSAGE(" ComputeState = READY_TO_COMPUTE");break;
  case COMPUTE_OK       : MESSAGE(" ComputeState = COMPUTE_OK");break;
  case FAILED_TO_COMPUTE: MESSAGE(" ComputeState = FAILED_TO_COMPUTE");break;
  }
}

//================================================================================
/*!
 * \brief Remove nodes and elements bound to submesh
  * \param subMesh - submesh containing nodes and elements
 */
//================================================================================

static void cleanSubMesh( SMESH_subMesh * subMesh )
{
  if (subMesh) {
    if (SMESHDS_SubMesh * subMeshDS = subMesh->GetSubMeshDS())
    {
      SMESHDS_Mesh * meshDS = subMesh->GetFather()->GetMeshDS();
      smIdType nbElems = subMeshDS->NbElements();
      if ( nbElems > 0 )
        for ( SMDS_ElemIteratorPtr ite = subMeshDS->GetElements(); ite->more(); )
          meshDS->RemoveFreeElement( ite->next(), subMeshDS );

      smIdType nbNodes = subMeshDS->NbNodes();
      if ( nbNodes > 0 )
        for ( SMDS_NodeIteratorPtr itn = subMeshDS->GetNodes(); itn->more() ; )
        {
          const SMDS_MeshNode * node = itn->next();
          if ( node->NbInverseElements() == 0 )
            meshDS->RemoveFreeNode( node, subMeshDS );
          else // for StdMeshers_CompositeSegment_1D: node in one submesh, edge in another
            meshDS->RemoveNode( node );
        }
      subMeshDS->Clear();
    }
  }
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool SMESH_subMesh::ComputeStateEngine(compute_event event)
{
  switch ( event ) {
  case MODIF_ALGO_STATE:
  case COMPUTE:
  case COMPUTE_SUBMESH:
    //case COMPUTE_CANCELED:
  case CLEAN:
    //case SUBMESH_COMPUTED:
    //case SUBMESH_RESTORED:
    //case SUBMESH_LOADED:
    //case MESH_ENTITY_REMOVED:
    //case CHECK_COMPUTE_STATE:
    _computeError.reset(); break;
  default:;
  }

  if ( event == CLEAN )
    _alwaysComputed = false; // Unset 'true' set by MergeNodes() (issue 0022182)

  if (_subShape.ShapeType() == TopAbs_VERTEX)
  {
    _computeState = READY_TO_COMPUTE;
    SMESHDS_SubMesh* smDS = GetSubMeshDS();
    if ( smDS && smDS->NbNodes() )
    {
      if ( event == CLEAN ) {
        cleanDependants();
        cleanSubMesh( this );
      }
      else
        _computeState = COMPUTE_OK;
    }
    else if (( event == COMPUTE || event == COMPUTE_SUBMESH )
             && !_alwaysComputed )
    {
      SMESH_MeshLocker myLocker(_father);
      const TopoDS_Vertex & V = TopoDS::Vertex( _subShape );
      gp_Pnt P = BRep_Tool::Pnt(V);
      if ( SMDS_MeshNode * n = _father->GetMeshDS()->AddNode(P.X(), P.Y(), P.Z()) ) {
        _father->GetMeshDS()->SetNodeOnVertex(n,_Id);
        _computeState = COMPUTE_OK;
      }
    }
    if ( event == MODIF_ALGO_STATE )
      cleanDependants();
    return true;
  }
  SMESH_Gen *gen = _father->GetGen();
  SMESH_Algo *algo = 0;
  bool ret = true;
  SMESH_Hypothesis::Hypothesis_Status hyp_status;
  //algo_state oldAlgoState = (algo_state) GetAlgoState();

  switch (_computeState)
  {

    // ----------------------------------------------------------------------

  case NOT_READY:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      algo = GetAlgo();
      if (algo && !algo->NeedDiscreteBoundary())
        cleanDependsOn( algo ); // clean sub-meshes with event CLEAN
      if ( _algoState == HYP_OK )
        _computeState = READY_TO_COMPUTE;
      break;
    case COMPUTE:               // nothing to do
    case COMPUTE_SUBMESH:
      break;
    case COMPUTE_CANCELED:      // nothing to do
      break;
    case CLEAN:
      cleanDependants();
      removeSubMeshElementsAndNodes();
      break;
    case SUBMESH_COMPUTED:      // nothing to do
      break;
    case SUBMESH_RESTORED:
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      break;
    case MESH_ENTITY_REMOVED:
      break;
    case SUBMESH_LOADED:
      loadDependentMeshes();
      ComputeSubMeshStateEngine( SUBMESH_LOADED );
      //break;
      // fall through
    case CHECK_COMPUTE_STATE:
      if ( IsMeshComputed() )
        _computeState = COMPUTE_OK;
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case READY_TO_COMPUTE:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      _computeState = NOT_READY;
      algo = GetAlgo();
      if (algo)
      {
        if (!algo->NeedDiscreteBoundary())
          cleanDependsOn( algo ); // clean sub-meshes with event CLEAN
        if ( _algoState == HYP_OK )
          _computeState = READY_TO_COMPUTE;
      }
      break;

    case COMPUTE_NOGEOM:  // no geometry; can be several algos
      if ( !_father->HasShapeToMesh() )
      {
        algo = GetAlgo(); // current algo
        if ( algo )
        {
          // apply algos in the order of increasing dimension
          std::list< const SMESHDS_Hypothesis * > algos = _father->GetHypothesisList( _subShape );
          for ( int t = SMESHDS_Hypothesis::ALGO_1D; t <= SMESHDS_Hypothesis::ALGO_3D; ++t )
          {
            std::list<const SMESHDS_Hypothesis *>::iterator al = algos.begin();
            for ( ; al != algos.end(); ++al )
              if ( (*al)->GetType() == t )
              {
                _algo = (SMESH_Algo*) *al;
                _computeState = READY_TO_COMPUTE;
                if ( !ComputeStateEngine( COMPUTE ))
                  break;
              }
          }
          _algo = algo; // restore
        }
        break;
      }
      // fall through
    case COMPUTE:
    case COMPUTE_SUBMESH:
      {
        algo = GetAlgo();
        ASSERT(algo);
        ret = algo->CheckHypothesis((*_father), _subShape, hyp_status);
        if (!ret)
        {
          MESSAGE("***** verify compute state *****");
          _computeState = NOT_READY;
          setAlgoState(MISSING_HYP);
          break;
        }
        TopoDS_Shape shape = _subShape;
        algo->setSubMeshesToCompute(this);
        // check submeshes needed
        // When computing in parallel mode we do not have a additional layer of submesh
        // The check should not be done in parallel as that check is not thread-safe
        if (_father->HasShapeToMesh() && (!_father->IsParallel() || shape.ShapeType() != _father->GetParallelElement() )) {
          bool subComputed = false, subFailed = false;
          if (!algo->OnlyUnaryInput()) {
            //  --- commented for bos#22320 to compute all sub-shapes at once if possible;
            //  --- in case COMPUTE_SUBMESH, set of sub-shapes is limited
            //  --- by calling SetAllowedSubShapes()
            // if ( event == COMPUTE )
            //   shape = getCollection( gen, algo, subComputed, subFailed, algo->SubMeshesToComput;
            // else
            //   subComputed = SubMeshesComputed( & subFailed );
            shape = getCollection( gen, algo, subComputed, subFailed, algo->SubMeshesToCompute());
          }
          else {
            subComputed = SubMeshesComputed();
          }
          ret = ( algo->NeedDiscreteBoundary() ? subComputed :
                  algo->SupportSubmeshes() ? !subFailed :
                  ( !subComputed || _father->IsNotConformAllowed() ));
          if (!ret)
          {
            _computeState = FAILED_TO_COMPUTE;
            if ( !algo->NeedDiscreteBoundary() && !subFailed )
              _computeError =
                SMESH_ComputeError::New(COMPERR_BAD_INPUT_MESH,
                                        "Unexpected computed sub-mesh",algo);
            break; // goto exit
          }
        }
        // Compute

        // to restore cout that may be redirected by algo
        std::streambuf* coutBuffer = std::cout.rdbuf();

        //cleanDependants(); for "UseExisting_*D" algos
        //removeSubMeshElementsAndNodes();
        loadDependentMeshes();
        ret = false;
        _computeState = FAILED_TO_COMPUTE;
        _computeError = SMESH_ComputeError::New(COMPERR_OK,"",algo);
        try {
          OCC_CATCH_SIGNALS;

          algo->InitComputeError();

          MemoryReserve aMemoryReserve;
          SMDS_Mesh::CheckMemory();
          Kernel_Utils::Localizer loc;
          if ( !_father->HasShapeToMesh() ) // no shape
          {
            SMESH_MesherHelper helper( *_father );
            helper.SetSubShape( shape );
            helper.SetElementsOnShape( true );
            ret = algo->Compute(*_father, &helper );
          }
          else
          {
            ret = algo->Compute((*_father), shape);
          }
          // algo can set _computeError of submesh
          _computeError = SMESH_ComputeError::Worst( _computeError, algo->GetComputeError() );
        }
        catch ( ::SMESH_ComputeError& comperr ) {
          MESSAGE(" SMESH_ComputeError caught");
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          *_computeError = comperr;
        }
        catch ( std::bad_alloc& exc ) {
          MESSAGE("std::bad_alloc thrown inside algo->Compute()");
          if ( _computeError ) {
            _computeError->myName = COMPERR_MEMORY_PB;
          }
          cleanSubMesh( this );
          throw exc;
        }
        catch ( Standard_OutOfMemory& exc ) {
          MESSAGE("Standard_OutOfMemory thrown inside algo->Compute()");
          if ( _computeError ) {
            _computeError->myName = COMPERR_MEMORY_PB;
          }
          cleanSubMesh( this );
          throw std::bad_alloc();
        }
        catch (Standard_Failure& ex) {
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          _computeError->myName    = COMPERR_OCC_EXCEPTION;
          _computeError->myComment += ex.DynamicType()->Name();
          if ( ex.GetMessageString() && strlen( ex.GetMessageString() )) {
            _computeError->myComment += ": ";
            _computeError->myComment += ex.GetMessageString();
          }
        }
        catch ( SALOME_Exception& S_ex ) {
          const int skipSalomeShift = 7; /* to skip "Salome " of
                                            "Salome Exception" prefix returned
                                            by SALOME_Exception::what() */
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          _computeError->myName    = COMPERR_SLM_EXCEPTION;
          _computeError->myComment = S_ex.what() + skipSalomeShift;
        }
        catch ( std::exception& exc ) {
          if ( !_computeError ) _computeError = SMESH_ComputeError::New();
          _computeError->myName    = COMPERR_STD_EXCEPTION;
          _computeError->myComment = exc.what();
        }
        catch ( ... ) {
          if ( _computeError )
            _computeError->myName = COMPERR_EXCEPTION;
          else
            ret = false;
        }
        std::cout.rdbuf( coutBuffer ); // restore cout that could be redirected by algo

        // check if an error reported on any sub-shape
        bool isComputeErrorSet = !checkComputeError( algo, ret, shape );
        if ( isComputeErrorSet )
          ret = false;
        // check if anything was built
        TopExp_Explorer subS(shape, _subShape.ShapeType());
        if ( ret )
        {
          for (; ret && subS.More(); subS.Next())
            if ( !_father->GetSubMesh( subS.Current() )->IsMeshComputed() &&
                 ( _subShape.ShapeType() != TopAbs_EDGE ||
                   !algo->isDegenerated( TopoDS::Edge( subS.Current() ))))
              ret = false;
        }
#ifdef PRINT_WHO_COMPUTE_WHAT
        for (subS.ReInit(); subS.More(); subS.Next())
        {
          SMESH_MeshLocker myLocker(_father);
          const std::list <const SMESHDS_Hypothesis *> & hyps =
              _algo->GetUsedHypothesis( *_father, _subShape );
          SMESH_Comment hypStr;
          if ( !hyps.empty() )
          {
            hypStr << hyps.front()->GetName() << " ";
            ((SMESHDS_Hypothesis*)hyps.front())->SaveTo( hypStr.Stream() );
            hypStr << " ";
          }
          cout << _father->GetSubMesh( subS.Current() )->GetId()
               << " " << ( ret ? "OK" : "FAIL" )
               << " " << _algo->GetName()
               << " " << hypStr << endl;
        }
#endif
        // Set _computeError
        if ( !ret && !isComputeErrorSet )
        {
          for ( subS.ReInit(); subS.More(); subS.Next() )
          {
            SMESH_subMesh* sm = _father->GetSubMesh( subS.Current() );
            if ( !sm->IsMeshComputed() )
            {
              if ( !sm->_computeError )
                sm->_computeError = SMESH_ComputeError::New();
              if ( sm->_computeError->IsOK() )
                sm->_computeError->myName = COMPERR_ALGO_FAILED;
              sm->_computeState = FAILED_TO_COMPUTE;
              sm->_computeError->myAlgo = algo;
            }
          }
        }
        if ( ret && _computeError && _computeError->myName != COMPERR_WARNING )
        {
          _computeError.reset();
        }

        // transform errors into warnings if it is caused by mesh edition (imp 0023068)
        if (!ret && _father->GetIsModified() )
        {
          for (subS.ReInit(); subS.More(); subS.Next())
          {
            SMESH_subMesh* sm = _father->GetSubMesh( subS.Current() );
            if ( !sm->IsMeshComputed() && sm->_computeError )
            {
              // check if there is a VERTEX w/o nodes
              // with READY_TO_COMPUTE state (after MergeNodes())
              SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(false,false);
              while ( smIt->more() )
              {
                SMESH_subMesh * vertSM = smIt->next();
                if ( vertSM->_subShape.ShapeType() != TopAbs_VERTEX ) break;
                if ( vertSM->GetComputeState() == READY_TO_COMPUTE )
                {
                  SMESHDS_SubMesh * ds = vertSM->GetSubMeshDS();
                  if ( !ds || ds->NbNodes() == 0 )
                  {
                    sm->_computeState = READY_TO_COMPUTE;
                    sm->_computeError->myName = COMPERR_WARNING;
                    break;
                  }
                }
              }
            }
          }
        }

        // send event SUBMESH_COMPUTED
        if ( ret ) {
          if ( !algo->NeedDiscreteBoundary() )
            // send SUBMESH_COMPUTED to dependants of all sub-meshes of shape
            for (subS.ReInit(); subS.More(); subS.Next())
            {
              SMESH_subMesh* sm = _father->GetSubMesh( subS.Current() );
              SMESH_subMeshIteratorPtr smIt = sm->getDependsOnIterator(false,false);
              while ( smIt->more() ) {
                sm = smIt->next();
                if ( sm->GetSubShape().ShapeType() == TopAbs_VERTEX )
                  sm->updateDependantsState( SUBMESH_COMPUTED );
                else
                  break;
              }
            }
          else
            updateDependantsState( SUBMESH_COMPUTED );
        }
        // let algo clear its data gathered while algo->Compute()
        algo->CheckHypothesis((*_father), _subShape, hyp_status);
      }
      break;
    case COMPUTE_CANCELED:               // nothing to do
      break;
    case CLEAN:
      cleanDependants();
      removeSubMeshElementsAndNodes();
      _computeState = NOT_READY;
      algo = GetAlgo();
      if (algo)
      {
        ret = algo->CheckHypothesis((*_father), _subShape, hyp_status);
        if (ret)
          _computeState = READY_TO_COMPUTE;
        else
          setAlgoState(MISSING_HYP);
      }
      break;
    case SUBMESH_COMPUTED:      // nothing to do
      break;
    case SUBMESH_RESTORED:
      // check if a mesh is already computed that may
      // happen after retrieval from a file
      ComputeStateEngine( CHECK_COMPUTE_STATE );
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      algo = GetAlgo();
      if (algo) algo->SubmeshRestored( this );
      break;
    case MESH_ENTITY_REMOVED:
      break;
    case SUBMESH_LOADED:
      loadDependentMeshes();
      ComputeSubMeshStateEngine( SUBMESH_LOADED );
      //break;
      // fall through
    case CHECK_COMPUTE_STATE:
      if ( IsMeshComputed() )
        _computeState = COMPUTE_OK;
      else if ( _computeError && _computeError->IsKO() )
        _computeState = FAILED_TO_COMPUTE;
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case COMPUTE_OK:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      ComputeStateEngine( CLEAN );
      algo = GetAlgo();
      if (algo && !algo->NeedDiscreteBoundary())
        cleanDependsOn( algo ); // clean sub-meshes with event CLEAN
      break;
    case COMPUTE:               // nothing to do
      break;
    case COMPUTE_SUBMESH:       // nothing to do
      break;
    case COMPUTE_CANCELED:      // nothing to do
      break;
    case CLEAN:
      cleanDependants();  // clean sub-meshes, dependent on this one, with event CLEAN
      removeSubMeshElementsAndNodes();
      _computeState = NOT_READY;
      if ( _algoState == HYP_OK )
        _computeState = READY_TO_COMPUTE;
      break;
    case SUBMESH_COMPUTED:      // nothing to do
      break;
    case SUBMESH_RESTORED:
      ComputeStateEngine( CHECK_COMPUTE_STATE );
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      algo = GetAlgo();
      if (algo) algo->SubmeshRestored( this );
      break;
    case MESH_ENTITY_REMOVED:
      updateDependantsState    ( CHECK_COMPUTE_STATE );
      ComputeStateEngine       ( CHECK_COMPUTE_STATE );
      ComputeSubMeshStateEngine( CHECK_COMPUTE_STATE );
      break;
    case CHECK_COMPUTE_STATE:
      if ( !IsMeshComputed() ) {
        if (_algoState == HYP_OK)
          _computeState = READY_TO_COMPUTE;
        else
          _computeState = NOT_READY;
      }
      break;
    case SUBMESH_LOADED:
      // already treated event, thanks to which _computeState == COMPUTE_OK
      break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------

  case FAILED_TO_COMPUTE:
    switch (event)
    {
    case MODIF_ALGO_STATE:
      if ( !IsEmpty() )
        ComputeStateEngine( CLEAN );
      algo = GetAlgo();
      if (algo && !algo->NeedDiscreteBoundary())
        cleanDependsOn( algo ); // clean sub-meshes with event CLEAN
      if (_algoState == HYP_OK)
        _computeState = READY_TO_COMPUTE;
      else
        _computeState = NOT_READY;
      break;
    case COMPUTE:        // nothing to do
    case COMPUTE_SUBMESH:
      break;
    case COMPUTE_CANCELED:
      {
        algo = GetAlgo();
        algo->CancelCompute();
      }
      break;
    case CLEAN:
      cleanDependants(); // submeshes dependent on me should be cleaned as well
      removeSubMeshElementsAndNodes();
      break;
    case SUBMESH_COMPUTED:      // allow retry compute
      if ( IsEmpty() ) // 23061
      {
        if (_algoState == HYP_OK)
          _computeState = READY_TO_COMPUTE;
        else
          _computeState = NOT_READY;
      }
      break;
    case SUBMESH_RESTORED:
      ComputeSubMeshStateEngine( SUBMESH_RESTORED );
      break;
    case MESH_ENTITY_REMOVED:
      break;
    case CHECK_COMPUTE_STATE:
      if ( IsMeshComputed() )
        _computeState = COMPUTE_OK;
      else
        if (_algoState == HYP_OK)
          _computeState = READY_TO_COMPUTE;
        else
          _computeState = NOT_READY;
      break;
    // case SUBMESH_LOADED:
    //   break;
    default:
      ASSERT(0);
      break;
    }
    break;

    // ----------------------------------------------------------------------
  default:
    ASSERT(0);
    break;
  }

  notifyListenersOnEvent( event, COMPUTE_EVENT );

  return ret;
}


//=============================================================================
/*!
 *
 */
//=============================================================================

bool SMESH_subMesh::Evaluate(MapShapeNbElems& aResMap)
{
  _computeError.reset();

  bool ret = true;

  if (_subShape.ShapeType() == TopAbs_VERTEX) {
    vector<smIdType> aVec(SMDSEntity_Last,0);
    aVec[SMDSEntity_Node] = 1;
    aResMap.insert(make_pair(this,aVec));
    return ret;
  }

  //SMESH_Gen *gen = _father->GetGen();
  SMESH_Algo *algo = 0;
  SMESH_Hypothesis::Hypothesis_Status hyp_status;

  algo = GetAlgo();
  if( algo && !aResMap.count( this ))
  {
    ret = algo->CheckHypothesis((*_father), _subShape, hyp_status);
    if (!ret) return false;

    if (_father->HasShapeToMesh() && algo->NeedDiscreteBoundary() )
    {
      // check submeshes needed
      bool subMeshEvaluated = true;
      int dimToCheck = SMESH_Gen::GetShapeDim( _subShape ) - 1;
      SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,/*complexShapeFirst=*/true);
      while ( smIt->more() && subMeshEvaluated )
      {
        SMESH_subMesh* sm = smIt->next();
        int dim = SMESH_Gen::GetShapeDim( sm->GetSubShape() );
        if (dim < dimToCheck) break; // the rest subMeshes are all of less dimension
        const vector<smIdType> & nbs = aResMap[ sm ];
        subMeshEvaluated = (std::accumulate( nbs.begin(), nbs.end(), 0 ) > 0 );
      }
      if ( !subMeshEvaluated )
        return false;
    }
    _computeError = SMESH_ComputeError::New(COMPERR_OK,"",algo);

    if ( IsMeshComputed() )
    {
      vector<smIdType> & nbEntities = aResMap[ this ];
      nbEntities.resize( SMDSEntity_Last, 0 );
      if ( SMESHDS_SubMesh* sm = GetSubMeshDS() )
      {
        nbEntities[ SMDSEntity_Node ] = sm->NbNodes();
        SMDS_ElemIteratorPtr   elemIt = sm->GetElements();
        while ( elemIt->more() )
          nbEntities[ elemIt->next()->GetEntityType() ]++;
      }
    }
    else
    {
      ret = algo->Evaluate((*_father), _subShape, aResMap);
    }
    aResMap.insert( make_pair( this,vector<smIdType>(0)));
  }

  return ret;
}


//=======================================================================
/*!
 * \brief Update compute_state by _computeError and send proper events to
 * dependent submeshes
  * \retval bool - true if _computeError is NOT set
 */
//=======================================================================

bool SMESH_subMesh::checkComputeError(SMESH_Algo*         theAlgo,
                                      const bool          theComputeOK,
                                      const TopoDS_Shape& theShape)
{
  bool noErrors = true;

  if ( !theShape.IsNull() )
  {
    // Check state of submeshes
    if ( !theAlgo->NeedDiscreteBoundary())
    {
      SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
      while ( smIt->more() )
        if ( !smIt->next()->checkComputeError( theAlgo, theComputeOK ))
          noErrors = false;
    }

    // Check state of neighbours
    if ( !theAlgo->OnlyUnaryInput() &&
         theShape.ShapeType() == TopAbs_COMPOUND &&
         !theShape.IsSame( _subShape ))
    {
      for (TopoDS_Iterator subIt( theShape ); subIt.More(); subIt.Next()) {
        SMESH_subMesh* sm = _father->GetSubMesh( subIt.Value() );
        if ( sm != this ) {
          if ( !sm->checkComputeError( theAlgo, theComputeOK, sm->GetSubShape() ))
            noErrors = false;
          updateDependantsState( SUBMESH_COMPUTED ); // send event SUBMESH_COMPUTED
        }
      }
    }
  }
  {

    // Set my _computeState

    if ( !_computeError || _computeError->IsOK() )
    {
      // no error description is set to this sub-mesh, check if any mesh is computed
      _computeState = IsMeshComputed() ? COMPUTE_OK : FAILED_TO_COMPUTE;
      if ( _computeState != COMPUTE_OK )
      {
        if ( _subShape.ShapeType() == TopAbs_EDGE &&
             SMESH_Algo::isDegenerated( TopoDS::Edge( _subShape )) )
          _computeState = COMPUTE_OK;
        else if ( theComputeOK )
          _computeError = SMESH_ComputeError::New(COMPERR_NO_MESH_ON_SHAPE,"",theAlgo);
      }
    }

    if ( _computeError && !_computeError->IsOK() )
    {
      if ( !_computeError->myAlgo )
        _computeError->myAlgo = theAlgo;

      // Show error
      SMESH_Comment text;
      text << theAlgo->GetName() << " failed on sub-shape #" << _Id << " with error ";
      if (_computeError->IsCommon() )
        text << _computeError->CommonName();
      else
        text << _computeError->myName;
      if ( _computeError->myComment.size() > 0 )
        text << " \"" << _computeError->myComment << "\"";

      INFOS( text );

      _computeState = _computeError->IsKO() ? FAILED_TO_COMPUTE : COMPUTE_OK;

      noErrors = false;
    }
  }
  return noErrors;
}

//=======================================================================
//function : updateSubMeshState
//purpose  :
//=======================================================================

void SMESH_subMesh::updateSubMeshState(const compute_state theState)
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(false,false);
  while ( smIt->more() )
    smIt->next()->_computeState = theState;
}

//=======================================================================
//function : ComputeSubMeshStateEngine
//purpose  :
//=======================================================================

void SMESH_subMesh::ComputeSubMeshStateEngine(compute_event event, const bool includeSelf)
{
  SMESH_subMeshIteratorPtr smIt = getDependsOnIterator(includeSelf,false);
  while ( smIt->more() )
    smIt->next()->ComputeStateEngine(event);
}

//=======================================================================
//function : updateDependantsState
//purpose  :
//=======================================================================

void SMESH_subMesh::updateDependantsState(const compute_event theEvent)
{
  const std::vector< SMESH_subMesh * > & ancestors = GetAncestors();
  for ( size_t iA = 0; iA < ancestors.size(); ++iA )
  {
    ancestors[ iA ]->ComputeStateEngine( theEvent );
  }
}

//=======================================================================
//function : cleanDependants
//purpose  :
//=======================================================================

void SMESH_subMesh::cleanDependants()
{
  int dimToClean = SMESH_Gen::GetShapeDim( _subShape ) + 1;

  const std::vector< SMESH_subMesh * > & ancestors = GetAncestors();
  for ( size_t iA = 0; iA < ancestors.size(); ++iA )
  {
    const TopoDS_Shape& ancestor = ancestors[ iA ]->GetSubShape();
    if ( SMESH_Gen::GetShapeDim( ancestor ) == dimToClean )
    {
      // PAL8021. do not go upper than SOLID, else ComputeStateEngine(CLEAN)
      // will erase mesh on other shapes in a compound
      if ( ancestor.ShapeType() >= TopAbs_SOLID &&
           !ancestors[ iA ]->IsEmpty() )  // prevent infinite CLEAN via event lesteners
        ancestors[ iA ]->ComputeStateEngine(CLEAN);
    }
  }
}

//=======================================================================
//function : removeSubMeshElementsAndNodes
//purpose  :
//=======================================================================

void SMESH_subMesh::removeSubMeshElementsAndNodes()
{
  cleanSubMesh( this );

  // algo may bind a submesh not to _subShape, eg 3D algo
  // sets nodes on SHELL while _subShape may be SOLID

  int dim = SMESH_Gen::GetShapeDim( _subShape );
  int type = _subShape.ShapeType() + 1;
  for ( ; type <= TopAbs_EDGE; type++) {
    if ( dim == SMESH_Gen::GetShapeDim( (TopAbs_ShapeEnum) type ))
    {
      TopExp_Explorer exp( _subShape, (TopAbs_ShapeEnum) type );
      for ( ; exp.More(); exp.Next() )
        cleanSubMesh( _father->GetSubMeshContaining( exp.Current() ));
    }
    else
      break;
  }
}

//=======================================================================
//function : getCollection
//purpose  : return a shape containing all sub-shapes of the MainShape that can be
//           meshed at once along with _subShape
//=======================================================================

TopoDS_Shape SMESH_subMesh::getCollection(SMESH_Gen * /*theGen*/,
                                          SMESH_Algo* theAlgo,
                                          bool &      theSubComputed,
                                          bool &      theSubFailed,
                                          std::vector<SMESH_subMesh*>& theSubs)
{
  theSubComputed = SubMeshesComputed( & theSubFailed );

  TopoDS_Shape mainShape = _father->GetMeshDS()->ShapeToMesh();

  if ( mainShape.IsSame( _subShape ))
    return _subShape;

  const bool skipAuxHyps = false;
  list<const SMESHDS_Hypothesis*> usedHyps =
    theAlgo->GetUsedHypothesis( *_father, _subShape, skipAuxHyps ); // copy
  std::list < TopoDS_Shape >  assiShapes = theAlgo->GetAssignedShapes();

  // put in a compound all shapes with the same hypothesis assigned
  // and a good ComputeState

  TopoDS_Compound aCompound;
  BRep_Builder aBuilder;
  aBuilder.MakeCompound( aCompound );

  theSubs.clear();

  SMESH_subMeshIteratorPtr smIt = _father->GetSubMesh( mainShape )->getDependsOnIterator(false);
  while ( smIt->more() )
  {
    SMESH_subMesh* subMesh = smIt->next();
    const TopoDS_Shape&  S = subMesh->_subShape;

    if ( S.ShapeType() != this->_subShape.ShapeType() ){
      continue;
    }
    if ( _allowedSubShapes && !_allowedSubShapes->IsEmpty() && !_allowedSubShapes->Contains( S )){
      continue;
    }
    if ( subMesh == this )
    {
      aBuilder.Add( aCompound, S );
      theSubs.push_back( subMesh );
    }
    else if ( subMesh->GetComputeState() == READY_TO_COMPUTE )
    {
      SMESH_Algo* anAlgo = subMesh->GetAlgo();

      if (( anAlgo->IsSameName( *theAlgo )) && // same algo
          ( anAlgo->GetUsedHypothesis( *_father, S, skipAuxHyps ) == usedHyps ) && // same hyps
          ( anAlgo->GetAssignedShapes() == assiShapes ) && // on same sub-shapes
          ( _algoShape == subMesh->_algoShape ))
      {
        aBuilder.Add( aCompound, S );
        if ( !subMesh->SubMeshesComputed() )
          theSubComputed = false;
        theSubs.push_back( subMesh );
      }
    }
  }

  return theSubs.size() == 1 ? theSubs[0]->GetSubShape() : aCompound;
}

//=======================================================================
//function : getSimilarAttached
//purpose  : return a hypothesis attached to theShape.
//           If theHyp is provided, similar but not same hypotheses
//           is returned; else only applicable ones having theHypType
//           is returned
//=======================================================================

const SMESH_Hypothesis* SMESH_subMesh::getSimilarAttached(const TopoDS_Shape&      theShape,
                                                          const SMESH_Hypothesis * theHyp,
                                                          const int                theHypType)
{
  SMESH_HypoFilter hypoKind;
  hypoKind.Init( hypoKind.HasType( theHyp ? theHyp->GetType() : theHypType ));
  if ( theHyp ) {
    hypoKind.And   ( hypoKind.HasDim( theHyp->GetDim() ));
    hypoKind.AndNot( hypoKind.Is( theHyp ));
    if ( theHyp->IsAuxiliary() )
      hypoKind.And( hypoKind.HasName( theHyp->GetName() ));
    else
      hypoKind.AndNot( hypoKind.IsAuxiliary());
  }
  else {
    hypoKind.And( hypoKind.IsApplicableTo( theShape ));
  }

  return _father->GetHypothesis( theShape, hypoKind, false );
}

//=======================================================================
//function : CheckConcurrentHypothesis
//purpose  : check if there are several applicable hypothesis attached to
//           ancestors
//=======================================================================

SMESH_Hypothesis::Hypothesis_Status
  SMESH_subMesh::CheckConcurrentHypothesis( SMESH_Hypothesis* theHypothesis)
{
  // is there local hypothesis on me?
  if ( getSimilarAttached( _subShape, theHypothesis ) )
    return SMESH_Hypothesis::HYP_OK;


  TopoDS_Shape aPrevWithHyp;
  const SMESH_Hypothesis* aPrevHyp = 0;
  TopTools_ListIteratorOfListOfShape it( _father->GetAncestors( _subShape ));
  for (; it.More(); it.Next())
  {
    const TopoDS_Shape& ancestor = it.Value();
    const SMESH_Hypothesis* hyp = getSimilarAttached( ancestor, theHypothesis );
    if ( hyp )
    {
      if ( aPrevWithHyp.IsNull() || aPrevWithHyp.IsSame( ancestor ))
      {
        aPrevWithHyp = ancestor;
        aPrevHyp     = hyp;
      }
      else if ( aPrevWithHyp.ShapeType() == ancestor.ShapeType() && aPrevHyp != hyp )
        return SMESH_Hypothesis::HYP_CONCURRENT;
      else
        return SMESH_Hypothesis::HYP_OK;
    }
  }
  return SMESH_Hypothesis::HYP_OK;
}

//================================================================================
/*!
 * \brief Constructor of OwnListenerData
 */
//================================================================================

SMESH_subMesh::OwnListenerData::OwnListenerData( SMESH_subMesh* sm, EventListener* el):
  mySubMesh( sm ),
  myMeshID( sm ? sm->GetFather()->GetId() : -1 ),
  mySubMeshID( sm ? sm->GetId() : -1 ),
  myListener( el )
{
}

//================================================================================
/*!
 * \brief Sets an event listener and its data to a submesh
 * \param listener - the listener to store
 * \param data - the listener data to store
 * \param where - the submesh to store the listener and it's data
 *
 * It remembers the submesh where it puts the listener in order to delete
 * them when HYP_OK algo_state is lost
 * After being set, event listener is notified on each event of where submesh.
 */
//================================================================================

void SMESH_subMesh::SetEventListener(EventListener*     listener,
                                     EventListenerData* data,
                                     SMESH_subMesh*     where)
{
  if ( listener && where ) {
    where->setEventListener( listener, data );
    _ownListeners.push_back( OwnListenerData( where, listener ));
  }
}

//================================================================================
/*!
 * \brief Sets an event listener and its data to a submesh
 * \param listener - the listener to store
 * \param data - the listener data to store
 *
 * After being set, event listener is notified on each event of a submesh.
 */
//================================================================================

void SMESH_subMesh::setEventListener(EventListener*     listener,
                                     EventListenerData* data)
{
  map< EventListener*, EventListenerData* >::iterator l_d =
    _eventListeners.find( listener );
  if ( l_d != _eventListeners.end() ) {
    EventListenerData* curData = l_d->second;
    l_d->second = data;
    if ( curData && curData != data && curData->IsDeletable() )
      delete curData;
  }
  else
  {
    for ( l_d = _eventListeners.begin(); l_d != _eventListeners.end(); ++l_d )
      if ( listener->GetName() == l_d->first->GetName() )
      {
        EventListenerData* curData = l_d->second;
        l_d->second = 0;
        if ( curData && curData != data && curData->IsDeletable() )
          delete curData;
        if ( l_d->first != listener && l_d->first->IsDeletable() )
          delete l_d->first;
        _eventListeners.erase( l_d );
        break;
      }
    _eventListeners.insert( make_pair( listener, data ));
  }
}

//================================================================================
/*!
 * \brief Return an event listener data
 * \param listener - the listener whose data is
 * \param myOwn - if \c true, returns a listener set by this sub-mesh,
 *        else returns a listener listening to events of this sub-mesh
 * \retval EventListenerData* - found data, maybe NULL
 */
//================================================================================

EventListenerData* SMESH_subMesh::GetEventListenerData(EventListener* listener,
                                                       const bool     myOwn) const
{
  if ( myOwn )
  {
    list< OwnListenerData >::const_iterator d;
    for ( d = _ownListeners.begin(); d != _ownListeners.end(); ++d )
    {
      if ( d->myListener == listener && _father->MeshExists( d->myMeshID ))
        return d->mySubMesh->GetEventListenerData( listener, !myOwn );
    }
  }
  else
  {
    map< EventListener*, EventListenerData* >::const_iterator l_d =
      _eventListeners.find( listener );
    if ( l_d != _eventListeners.end() )
      return l_d->second;
  }
  return 0;
}

//================================================================================
/*!
 * \brief Return an event listener data
 * \param listenerName - the listener name
 * \param myOwn - if \c true, returns a listener set by this sub-mesh,
 *        else returns a listener listening to events of this sub-mesh
 * \retval EventListenerData* - found data, maybe NULL
 */
//================================================================================

EventListenerData* SMESH_subMesh::GetEventListenerData(const string& listenerName,
                                                       const bool    myOwn) const
{
  if ( myOwn )
  {
    list< OwnListenerData >::const_iterator d;
    for ( d = _ownListeners.begin(); d != _ownListeners.end(); ++d )
    {
      if ( _father->MeshExists( d->myMeshID ) && listenerName == d->myListener->GetName())
        return d->mySubMesh->GetEventListenerData( listenerName, !myOwn );
    }
  }
  else
  {
    map< EventListener*, EventListenerData* >::const_iterator l_d = _eventListeners.begin();
    for ( ; l_d != _eventListeners.end(); ++l_d )
      if ( listenerName == l_d->first->GetName() )
        return l_d->second;
  }
  return 0;
}

//================================================================================
/*!
 * \brief Notify stored event listeners on the occurred event
 * \param event - algo_event or compute_event itself
 * \param eventType - algo_event or compute_event
 * \param hyp - hypothesis, if eventType is algo_event
 */
//================================================================================

void SMESH_subMesh::notifyListenersOnEvent( const int         event,
                                            const event_type  eventType,
                                            SMESH_Hypothesis* hyp)
{
  list< pair< EventListener*, EventListenerData* > > eventListeners( _eventListeners.begin(),
                                                                     _eventListeners.end());
  list< pair< EventListener*, EventListenerData* > >::iterator l_d = eventListeners.begin();
  for ( ; l_d != eventListeners.end(); ++l_d )
  {
    std::pair< EventListener*, EventListenerData* > li_da = *l_d;
    if ( !_eventListeners.count( li_da.first )) continue;

    if ( li_da.first->myBusySM.insert( this ).second )
    {
      const bool isDeletable = li_da.first->IsDeletable();

      li_da.first->ProcessEvent( event, eventType, this, li_da.second, hyp );

      if ( !isDeletable || _eventListeners.count( li_da.first ))
        li_da.first->myBusySM.erase( this ); // a listener is hopefully not dead
    }
  }
}

//================================================================================
/*!
 * \brief Unregister the listener and delete listener's data
 * \param listener - the event listener
 */
//================================================================================

void SMESH_subMesh::DeleteEventListener(EventListener* listener)
{
  map< EventListener*, EventListenerData* >::iterator l_d =
    _eventListeners.find( listener );
  if ( l_d != _eventListeners.end() && l_d->first )
  {
    if ( l_d->second && l_d->second->IsDeletable() )
    {
      delete l_d->second;
    }
    l_d->first->myBusySM.erase( this );
    if ( l_d->first->IsDeletable() )
    {
      l_d->first->BeforeDelete( this, l_d->second );
      delete l_d->first;
    }
    _eventListeners.erase( l_d );
  }
}

//================================================================================
/*!
 * \brief Delete event listeners depending on algo of this submesh
 */
//================================================================================

void SMESH_subMesh::deleteOwnListeners()
{
  list< OwnListenerData >::iterator d;
  for ( d = _ownListeners.begin(); d != _ownListeners.end(); ++d )
  {
    SMESH_Mesh* mesh = _father->FindMesh( d->myMeshID );
    if ( !mesh || !mesh->GetSubMeshContaining( d->mySubMeshID ))
      continue;
    d->mySubMesh->DeleteEventListener( d->myListener );
  }
  _ownListeners.clear();
}

//=======================================================================
//function : loadDependentMeshes
//purpose  : loads dependent meshes on SUBMESH_LOADED event
//=======================================================================

void SMESH_subMesh::loadDependentMeshes()
{
  list< OwnListenerData >::iterator d;
  for ( d = _ownListeners.begin(); d != _ownListeners.end(); ++d )
    if ( _father != d->mySubMesh->_father &&
         _father->FindMesh( d->myMeshID ))
      d->mySubMesh->_father->Load();

  // map< EventListener*, EventListenerData* >::iterator l_d = _eventListeners.begin();
  // for ( ; l_d != _eventListeners.end(); ++l_d )
  //   if ( l_d->second )
  //   {
  //     const list<SMESH_subMesh*>& smList = l_d->second->mySubMeshes;
  //     list<SMESH_subMesh*>::const_iterator sm = smList.begin();
  //     for ( ; sm != smList.end(); ++sm )
  //       if ( _father != (*sm)->_father )
  //         (*sm)->_father->Load();
  //   }
}

//================================================================================
/*!
 * \brief Do something on a certain event
 * \param event - algo_event or compute_event itself
 * \param eventType - algo_event or compute_event
 * \param subMesh - the submesh where the event occurs
 * \param data - listener data stored in the subMesh
 * \param hyp - hypothesis, if eventType is algo_event
 *
 * The base implementation translates CLEAN event to the subMesh
 * stored in listener data. Also it sends SUBMESH_COMPUTED event in case of
 * successful COMPUTE event.
 */
//================================================================================

void SMESH_subMeshEventListener::ProcessEvent(const int          event,
                                              const int          eventType,
                                              SMESH_subMesh*     subMesh,
                                              EventListenerData* data,
                                              const SMESH_Hypothesis*  /*hyp*/)
{
  if ( data && !data->mySubMeshes.empty() &&
       eventType == SMESH_subMesh::COMPUTE_EVENT)
  {
    ASSERT( data->mySubMeshes.front() != subMesh );
    list<SMESH_subMesh*>::iterator smIt = data->mySubMeshes.begin();
    list<SMESH_subMesh*>::iterator smEnd = data->mySubMeshes.end();
    switch ( event ) {
    case SMESH_subMesh::CLEAN:
      for ( ; smIt != smEnd; ++ smIt)
        (*smIt)->ComputeStateEngine( SMESH_subMesh::compute_event( event ));
      break;
    case SMESH_subMesh::COMPUTE:
    case SMESH_subMesh::COMPUTE_SUBMESH:
      if ( subMesh->GetComputeState() == SMESH_subMesh::COMPUTE_OK )
        for ( ; smIt != smEnd; ++ smIt)
          (*smIt)->ComputeStateEngine( SMESH_subMesh::SUBMESH_COMPUTED );
      break;
    default:;
    }
  }
}

namespace {

  //================================================================================
  /*!
   * \brief Iterator over submeshes and optionally prepended or appended one
   */
  //================================================================================

  struct _Iterator : public SMDS_Iterator<SMESH_subMesh*>
  {
    _Iterator(SMDS_Iterator<SMESH_subMesh*>* subIt,
              SMESH_subMesh*                 prepend,
              SMESH_subMesh*                 append): myAppend(append), myIt(subIt)
    {
      myCur = prepend ? prepend : myIt->more() ? myIt->next() : append;
      if ( myCur == append ) append = 0;
    }
    /// Return true if and only if there are other object in this iterator
    virtual bool more()
    {
      return myCur;
    }
    /// Return the current object and step to the next one
    virtual SMESH_subMesh* next()
    {
      SMESH_subMesh* res = myCur;
      if ( myIt->more() ) { myCur = myIt->next(); }
      else                { myCur = myAppend; myAppend = 0; }
      return res;
    }
    /// ~
    ~_Iterator()
    { delete myIt; }
    ///
    SMESH_subMesh                 *myAppend, *myCur;
    SMDS_Iterator<SMESH_subMesh*> *myIt;
  };
}

//================================================================================
/*!
 * \brief Return iterator on the submeshes this one depends on
 *  \param includeSelf - this submesh to be returned also
 *  \param reverse - if true, complex shape submeshes go first
 */
//================================================================================

SMESH_subMeshIteratorPtr SMESH_subMesh::getDependsOnIterator(const bool includeSelf,
                                                             const bool reverse) const
{
  SMESH_subMesh *me = (SMESH_subMesh*) this;
  SMESH_subMesh *prepend=0, *append=0;
  if ( includeSelf ) {
    if ( reverse ) prepend = me;
    else            append = me;
  }
  typedef map < int, SMESH_subMesh * > TMap;
  if ( reverse )
  {
    return SMESH_subMeshIteratorPtr
      ( new _Iterator( new SMDS_mapReverseIterator<TMap>( me->DependsOn() ), prepend, append ));
  }
  {
    return SMESH_subMeshIteratorPtr
      ( new _Iterator( new SMDS_mapIterator<TMap>( me->DependsOn() ), prepend, append ));
  }
}

//================================================================================
/*!
 * \brief Returns ancestor sub-meshes. Finds them if not yet found.
 */
//================================================================================

const std::vector< SMESH_subMesh * > & SMESH_subMesh::GetAncestors() const
{
  if ( _ancestors.empty() &&
       !_subShape.IsSame( _father->GetShapeToMesh() ))
  {
    const TopTools_ListOfShape& ancShapes = _father->GetAncestors( _subShape );

    SMESH_subMesh* me = const_cast< SMESH_subMesh* >( this );
    me->_ancestors.reserve( ancShapes.Extent() );

    // assure that all sub-meshes exist
    TopoDS_Shape mainShape = _father->GetShapeToMesh();
    if ( !mainShape.IsNull() )
      _father->GetSubMesh( mainShape )->DependsOn();

    TopTools_MapOfShape map;

    for ( TopTools_ListIteratorOfListOfShape it( ancShapes ); it.More(); it.Next() )
      if ( SMESH_subMesh* sm = _father->GetSubMeshContaining( it.Value() ))
        if ( map.Add( it.Value() ))
          me->_ancestors.push_back( sm );
  }

  return _ancestors;
}

//================================================================================
/*!
 * \brief Clears the vector of ancestor sub-meshes
 */
//================================================================================

void SMESH_subMesh::ClearAncestors()
{
  _ancestors.clear();
}

//================================================================================
/*!
 * \brief  Find common submeshes (based on shared sub-shapes with other
  * \param theOther submesh to check
  * \param theSetOfCommon set of common submesh
 */
//================================================================================

bool SMESH_subMesh::FindIntersection(const SMESH_subMesh*            theOther,
                                     std::set<const SMESH_subMesh*>& theSetOfCommon ) const
{
  size_t oldNb = theSetOfCommon.size();

  // check main submeshes
  const map <int, SMESH_subMesh*>::const_iterator otherEnd = theOther->_mapDepend.end();
  if ( theOther->_mapDepend.find(this->GetId()) != otherEnd )
    theSetOfCommon.insert( this );
  if ( _mapDepend.find(theOther->GetId()) != _mapDepend.end() )
    theSetOfCommon.insert( theOther );

  // check common submeshes
  map <int, SMESH_subMesh*>::const_iterator mapIt = _mapDepend.begin();
  for( ; mapIt != _mapDepend.end(); mapIt++ )
    if ( theOther->_mapDepend.find((*mapIt).first) != otherEnd )
      theSetOfCommon.insert( (*mapIt).second );
  return oldNb < theSetOfCommon.size();
}
