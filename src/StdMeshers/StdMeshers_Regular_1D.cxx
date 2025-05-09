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

//  File   : StdMeshers_Regular_1D.cxx
//           Moved here from SMESH_Regular_1D.cxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH
//
#include "StdMeshers_Regular_1D.hxx"

#include "SMDS_MeshElement.hxx"
#include "SMDS_MeshNode.hxx"
#include "SMESHDS_Mesh.hxx"
#include "SMESH_Comment.hxx"
#include "SMESH_Gen.hxx"
#include "SMESH_HypoFilter.hxx"
#include "SMESH_Mesh.hxx"
#include "SMESH_subMesh.hxx"
#include "SMESH_subMeshEventListener.hxx"
#include "StdMeshers_Adaptive1D.hxx"
#include "StdMeshers_Arithmetic1D.hxx"
#include "StdMeshers_AutomaticLength.hxx"
#include "StdMeshers_Geometric1D.hxx"
#include "StdMeshers_Deflection1D.hxx"
#include "StdMeshers_Distribution.hxx"
#include "StdMeshers_FixedPoints1D.hxx"
#include "StdMeshers_LocalLength.hxx"
#include "StdMeshers_MaxLength.hxx"
#include "StdMeshers_NumberOfSegments.hxx"
#include "StdMeshers_Propagation.hxx"
#include "StdMeshers_SegmentLengthAroundVertex.hxx"
#include "StdMeshers_StartEndLength.hxx"

#include <Utils_SALOME_Exception.hxx>
#include <utilities.h>

#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GCPnts_AbscissaPoint.hxx>
#include <GCPnts_UniformAbscissa.hxx>
#include <GCPnts_UniformDeflection.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>

#include <string>
#include <limits>

using namespace std;
using namespace StdMeshers;

//=============================================================================
/*!
 *
 */
//=============================================================================

StdMeshers_Regular_1D::StdMeshers_Regular_1D(int         hypId,
                                             SMESH_Gen * gen)
  :SMESH_1D_Algo( hypId, gen )
{
  _name = "Regular_1D";
  _shapeType = (1 << TopAbs_EDGE);
  _fpHyp = 0;

  _compatibleHypothesis.push_back("LocalLength");
  _compatibleHypothesis.push_back("MaxLength");
  _compatibleHypothesis.push_back("NumberOfSegments");
  _compatibleHypothesis.push_back("StartEndLength");
  _compatibleHypothesis.push_back("Deflection1D");
  _compatibleHypothesis.push_back("Arithmetic1D");
  _compatibleHypothesis.push_back("GeometricProgression");
  _compatibleHypothesis.push_back("FixedPoints1D");
  _compatibleHypothesis.push_back("AutomaticLength");
  _compatibleHypothesis.push_back("Adaptive1D");
  // auxiliary:
  _compatibleHypothesis.push_back("QuadraticMesh");
  _compatibleHypothesis.push_back("Propagation");
  _compatibleHypothesis.push_back("PropagOfDistribution");
}

//=============================================================================
/*!
 *
 */
//=============================================================================

StdMeshers_Regular_1D::~StdMeshers_Regular_1D()
{
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool StdMeshers_Regular_1D::CheckHypothesis( SMESH_Mesh&         aMesh,
                                             const TopoDS_Shape& aShape,
                                             Hypothesis_Status&  aStatus )
{
  _hypType        = NONE;
  _quadraticMesh  = false;
  _onlyUnaryInput = true;

  // check propagation in a redefined GetUsedHypothesis()
  const list <const SMESHDS_Hypothesis * > & hyps =
    GetUsedHypothesis(aMesh, aShape, /*ignoreAuxiliaryHyps=*/false);

  const SMESH_HypoFilter & propagFilter = StdMeshers_Propagation::GetFilter();

  // find non-auxiliary hypothesis
  const SMESHDS_Hypothesis *theHyp = 0;
  set< string > propagTypes;
  list <const SMESHDS_Hypothesis * >::const_iterator h = hyps.begin();
  for ( ; h != hyps.end(); ++h ) {
    if ( static_cast<const SMESH_Hypothesis*>(*h)->IsAuxiliary() ) {
      if ( strcmp( "QuadraticMesh", (*h)->GetName() ) == 0 )
        _quadraticMesh = true;
      if ( propagFilter.IsOk( static_cast< const SMESH_Hypothesis*>( *h ), aShape ))
        propagTypes.insert( (*h)->GetName() );
    }
    else {
      if ( !theHyp )
        theHyp = *h; // use only the first non-auxiliary hypothesis
    }
  }

  if ( !theHyp )
  {
    aStatus = SMESH_Hypothesis::HYP_MISSING;
    return false;  // can't work without a hypothesis
  }

  string hypName = theHyp->GetName();

  if ( !_mainEdge.IsNull() && _hypType == DISTRIB_PROPAGATION )
  {
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else if ( hypName == "LocalLength" )
  {
    const StdMeshers_LocalLength * hyp =
      dynamic_cast <const StdMeshers_LocalLength * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength();
    _value[ PRECISION_IND  ] = hyp->GetPrecision();
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 );
    _hypType = LOCAL_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "MaxLength" )
  {
    const StdMeshers_MaxLength * hyp =
      dynamic_cast <const StdMeshers_MaxLength * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength();
    if ( hyp->GetUsePreestimatedLength() ) {
      if ( int nbSeg = aMesh.GetGen()->GetBoundaryBoxSegmentation() )
        _value[ BEG_LENGTH_IND ] = aMesh.GetShapeDiagonalSize() / nbSeg;
    }
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 );
    _hypType = MAX_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "NumberOfSegments" )
  {
    const StdMeshers_NumberOfSegments * hyp =
      dynamic_cast <const StdMeshers_NumberOfSegments * >(theHyp);
    ASSERT(hyp);
    _ivalue[ NB_SEGMENTS_IND  ] = hyp->GetNumberOfSegments();
    ASSERT( _ivalue[ NB_SEGMENTS_IND ] > 0 );
    _ivalue[ DISTR_TYPE_IND ] = (int) hyp->GetDistrType();
    switch (_ivalue[ DISTR_TYPE_IND ])
    {
    case StdMeshers_NumberOfSegments::DT_Scale:
      _value[ SCALE_FACTOR_IND ] = hyp->GetScaleFactor();
      _revEdgesIDs = hyp->GetReversedEdges();
      break;
    case StdMeshers_NumberOfSegments::DT_TabFunc:
      _vvalue[ TAB_FUNC_IND ] = hyp->GetTableFunction();
      _revEdgesIDs = hyp->GetReversedEdges();
      break;
    case StdMeshers_NumberOfSegments::DT_ExprFunc:
      _svalue[ EXPR_FUNC_IND ] = hyp->GetExpressionFunction();
      _revEdgesIDs = hyp->GetReversedEdges();
      break;
    case StdMeshers_NumberOfSegments::DT_BetaLaw:
      _value[BETA_IND] = hyp->GetBeta();
      _revEdgesIDs = hyp->GetReversedEdges();
      break;
    case StdMeshers_NumberOfSegments::DT_Regular:
      break;
    default:
      ASSERT(0);
      break;
    }
    if (_ivalue[ DISTR_TYPE_IND ] == StdMeshers_NumberOfSegments::DT_TabFunc ||
        _ivalue[ DISTR_TYPE_IND ] == StdMeshers_NumberOfSegments::DT_ExprFunc)
        _ivalue[ CONV_MODE_IND ] = hyp->ConversionMode();
    _hypType = NB_SEGMENTS;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "Arithmetic1D" )
  {
    const StdMeshers_Arithmetic1D * hyp =
      dynamic_cast <const StdMeshers_Arithmetic1D * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength( true );
    _value[ END_LENGTH_IND ] = hyp->GetLength( false );
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 && _value[ END_LENGTH_IND ] > 0 );
    _hypType = ARITHMETIC_1D;

    _revEdgesIDs = hyp->GetReversedEdges();

    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "GeometricProgression" )
  {
    const StdMeshers_Geometric1D * hyp =
      dynamic_cast <const StdMeshers_Geometric1D * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetStartLength();
    _value[ END_LENGTH_IND ] = hyp->GetCommonRatio();
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 && _value[ END_LENGTH_IND ] > 0 );
    _hypType = GEOMETRIC_1D;

    _revEdgesIDs = hyp->GetReversedEdges();

    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "FixedPoints1D" ) {
    _fpHyp = dynamic_cast <const StdMeshers_FixedPoints1D*>(theHyp);
    ASSERT(_fpHyp);
    _hypType = FIXED_POINTS_1D;

    _revEdgesIDs = _fpHyp->GetReversedEdges();

    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "StartEndLength" )
  {
    const StdMeshers_StartEndLength * hyp =
      dynamic_cast <const StdMeshers_StartEndLength * >(theHyp);
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = hyp->GetLength( true );
    _value[ END_LENGTH_IND ] = hyp->GetLength( false );
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 && _value[ END_LENGTH_IND ] > 0 );
    _hypType = BEG_END_LENGTH;

    _revEdgesIDs = hyp->GetReversedEdges();

    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "Deflection1D" )
  {
    const StdMeshers_Deflection1D * hyp =
      dynamic_cast <const StdMeshers_Deflection1D * >(theHyp);
    ASSERT(hyp);
    _value[ DEFLECTION_IND ] = hyp->GetDeflection();
    ASSERT( _value[ DEFLECTION_IND ] > 0 );
    _hypType = DEFLECTION;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }

  else if ( hypName == "AutomaticLength" )
  {
    StdMeshers_AutomaticLength * hyp = const_cast<StdMeshers_AutomaticLength *>
      (dynamic_cast <const StdMeshers_AutomaticLength * >(theHyp));
    ASSERT(hyp);
    _value[ BEG_LENGTH_IND ] = _value[ END_LENGTH_IND ] = hyp->GetLength( &aMesh, aShape );
    ASSERT( _value[ BEG_LENGTH_IND ] > 0 );
    _hypType = MAX_LENGTH;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else if ( hypName == "Adaptive1D" )
  {
    _adaptiveHyp = dynamic_cast < const StdMeshers_Adaptive1D* >(theHyp);
    ASSERT(_adaptiveHyp);
    _hypType = ADAPTIVE;
    _onlyUnaryInput = false;
    aStatus = SMESH_Hypothesis::HYP_OK;
  }
  else
  {
    aStatus = SMESH_Hypothesis::HYP_INCOMPATIBLE;
  }

  if ( propagTypes.size() > 1 && aStatus == HYP_OK )
  {
    // detect concurrent Propagation hyps
    _usedHypList.clear();
    list< TopoDS_Shape > assignedTo;
    if ( aMesh.GetHypotheses( aShape, propagFilter, _usedHypList, true, &assignedTo ) > 1 )
    {
      // find most simple shape and a hyp on it
      int simpleShape = TopAbs_COMPOUND;
      const SMESHDS_Hypothesis* localHyp = 0;
      list< TopoDS_Shape >::iterator            shape = assignedTo.begin();
      list< const SMESHDS_Hypothesis *>::iterator hyp = _usedHypList.begin();
      for ( ; shape != assignedTo.end(); ++shape )
        if ( shape->ShapeType() > simpleShape )
        {
          simpleShape = shape->ShapeType();
          localHyp = (*hyp);
        }
      // check if there a different hyp on simpleShape
      shape = assignedTo.begin();
      hyp = _usedHypList.begin();
      for ( ; hyp != _usedHypList.end(); ++hyp, ++shape )
        if ( shape->ShapeType() == simpleShape &&
             !localHyp->IsSameName( **hyp ))
        {
          aStatus = HYP_INCOMPAT_HYPS;
          return error( SMESH_Comment("Hypotheses of both \"")
                        << StdMeshers_Propagation::GetName() << "\" and \""
                        << StdMeshers_PropagOfDistribution::GetName()
                        << "\" types can't be applied to the same edge");
        }
    }
  }

  return ( aStatus == SMESH_Hypothesis::HYP_OK );
}

static bool computeParamByFunc(Adaptor3d_Curve& C3d,
                               double first, double last, double length,
                               bool theReverse, smIdType nbSeg, Function& func,
                               list<double>& theParams)
{
  // never do this way
  //OSD::SetSignal( true );

  if ( nbSeg <= 0 )
    return false;

  smIdType nbPnt = 1 + nbSeg;
  vector<double> x( nbPnt, 0. );


  const double eps = Min( 1E-4, 0.01 / double( nbSeg ));

  if ( !buildDistribution( func, 0.0, 1.0, nbSeg, x, eps ))
     return false;

  // apply parameters in range [0,1] to the space of the curve
  double prevU = first;
  double  sign = 1.;
  if ( theReverse )
  {
    prevU = last;
    sign  = -1.;
  }

  for ( smIdType i = 1; i < nbSeg; i++ )
  {
    double curvLength = length * (x[i] - x[i-1]) * sign;
    double tol        = Min( Precision::Confusion(), curvLength / 100. );
    GCPnts_AbscissaPoint Discret( tol, C3d, curvLength, prevU );
    if ( !Discret.IsDone() )
      return false;
    double U = Discret.Parameter();
    if ( U > first && U < last )
      theParams.push_back( U );
    else
      return false;
    prevU = U;
  }
  if ( theReverse )
    theParams.reverse();

  return true;
}


//================================================================================
/*!
 * \brief adjust internal node parameters so that the last segment length == an
 *  \param a1 - the first segment length
 *  \param an - the last segment length
 *  \param U1 - the first edge parameter
 *  \param Un - the last edge parameter
 *  \param length - the edge length
 *  \param C3d - the edge curve
 *  \param theParams - internal node parameters to adjust
 *  \param adjustNeighbors2an - to adjust length of segments next to the last one
 *   and not to remove parameters
 */
//================================================================================

static void compensateError(double a1, double an,
                            double U1, double Un,
                            double            length,
                            Adaptor3d_Curve&  C3d,
                            list<double> &    theParams,
                            bool              adjustNeighbors2an = false)
{
  smIdType i, nPar = theParams.size();
  if ( a1 + an <= length && nPar > 1 )
  {
    bool reverse = ( U1 > Un );
    double tol   = Min( Precision::Confusion(), 0.01 * an );
    GCPnts_AbscissaPoint Discret( tol, C3d, reverse ? an : -an, Un );
    if ( !Discret.IsDone() )
      return;
    double Utgt = Discret.Parameter(); // target value of the last parameter
    list<double>::reverse_iterator itU = theParams.rbegin();
    double Ul = *itU++; // real value of the last parameter
    double dUn = Utgt - Ul; // parametric error of <an>
    double dU = Abs( Ul - *itU ); // parametric length of the last but one segment
    if ( Abs(dUn) <= 1e-3 * dU )
      return;
    if ( adjustNeighbors2an || Abs(dUn) < 0.5 * dU ) { // last segment is a bit shorter than it should
      // move the last parameter to the edge beginning
    }
    else {  // last segment is much shorter than it should -> remove the last param and
      theParams.pop_back(); nPar--; // move the rest points toward the edge end
      dUn = Utgt - theParams.back();
    }

    if ( !adjustNeighbors2an )
    {
      double q = dUn / ( Utgt - Un ); // (signed) factor of segment length change
      for ( itU = theParams.rbegin(), i = 1; i < nPar; i++ ) {
        double prevU = *itU;
        (*itU) += dUn;
        ++itU;
        dUn = q * (*itU - prevU) * (prevU-U1)/(Un-U1);
      }
    }
    else if ( nPar == 1 )
    {
      theParams.back() += dUn;
    }
    else
    {
      double q  =  dUn / double( nPar - 1 );
      theParams.back() += dUn;
      double sign = reverse ? -1 : 1;
      double prevU = theParams.back();
      itU = theParams.rbegin();
      for ( ++itU, i = 2; i < nPar; ++itU, i++ ) {
        double newU = *itU + dUn;
        if ( newU*sign < prevU*sign ) {
          prevU = *itU = newU;
          dUn -= q;
        }
        else { // set U between prevU and next valid param
          list<double>::reverse_iterator itU2 = itU;
          ++itU2;
          int nb = 2;
          while ( (*itU2)*sign > prevU*sign ) {
            ++itU2; ++nb;
          }
          dU = ( *itU2 - prevU ) / nb;
          while ( itU != itU2 ) {
            *itU += dU; ++itU;
          }
          break;
        }
      }
    }
  }
}


//================================================================================
/*!
 * \brief adjust internal node parameters so that the last segment length == an,
 *        and by distributing the error for the total length of curve segments
 *        in relation to the target length computed from the current parameters
 *  \param a1 - the first segment length
 *  \param an - the last segment length
 *  \param U1 - the first edge parameter
 *  \param Un - the last edge parameter
 *  \param length - the edge length
 *  \param C3d - the edge curve
 *  \param theParams - internal node parameters to adjust
 */
//================================================================================

static void distributeError(double a1, double an,
                            double U1, double Un,
                            double            length,
                            Adaptor3d_Curve&  C3d,
                            list<double> &    theParams)
{
  // Compute the error of the total length based in the current curve parameters
  double tol   = Min( Precision::Confusion(), 0.01 * Min(a1, an) );
  double totalLength = 0.0;
  double prevParam = U1;
  list<double> segLengths;
  list<double>::iterator itU = theParams.begin();
  for ( ; itU != theParams.end(); ++itU )
  {
    // Compute the curve length between two adjacent parameters and sum them up
    double curLength = GCPnts_AbscissaPoint::Length(C3d, prevParam, *itU, tol);
    segLengths.push_back(curLength);
    totalLength += curLength;
    prevParam = *itU;
  }
  // Calculate the error between the total length of all segments based on given parameters
  // and the target length of the edge itself
  double error = totalLength - length;
  // Compute the sum of all internal segments (= total computed length minus the length of
  // the start and end segments)
  double midLength = totalLength - (a1 + an);

  // We only need to distribute the error, if the current parametrization is not correct,
  // and if there are multiple internal segments
  smIdType nPar = theParams.size();
  if ( a1 + an <= length && nPar > 1 && fabs(error) > tol )
  {
    // Update the length of each internal segment (start and end length are given and not changed)
    double newTotalLength = 0.0;
    double newLength;
    double relError = error / midLength;
    list<double> newSegLengths;
    list<double>::iterator itL = segLengths.begin();
    for ( ; itL != segLengths.end(); ++itL )
    {
      // Do not update, but copy the first and the last segment lengths
      newLength = *itL;
      if (itL != segLengths.begin() && itL != --segLengths.end())
      {
        newLength -= newLength * relError;
      }
      newSegLengths.push_back(newLength);
      newTotalLength += newLength;
    }
    bool reverse = ( U1 > Un );

    // Update the parameters of the curve based on the new lengths
    double curveLength, tol2, U;
    double prevU = U1;
    itU = theParams.begin();
    itL = newSegLengths.begin();
    for ( ; itU != theParams.end(); ++itU, ++itL )
    {
      curveLength = (reverse ? -(*itL) : *itL);
      tol2        = Min( Precision::Confusion(), fabs(curveLength) / 100. );
      GCPnts_AbscissaPoint Discret( tol2, C3d, curveLength, prevU );
      if ( !Discret.IsDone() )
      {
        return;
      }
      U = Discret.Parameter();

      double sign = reverse ? -1 : 1;
      if ( sign*U1 < sign*U && sign*U < sign*Un )
      {
        *itU = U;
      }
      else
      {
        *itU = (sign*U >= sign*Un ? Un : U1);
        break;
      }
      prevU = U;
    }
  }
}


//================================================================================
/*!
 * \brief Class used to clean mesh on edges when 0D hyp modified.
 * Common approach doesn't work when 0D algo is missing because the 0D hyp is
 * considered as not participating in computation whereas it is used by 1D algo.
 */
//================================================================================

// struct VertexEventListener : public SMESH_subMeshEventListener
// {
//   VertexEventListener():SMESH_subMeshEventListener(0) // won't be deleted by submesh
//   {}
//   /*!
//    * \brief Clean mesh on edges
//    * \param event - algo_event or compute_event itself (of SMESH_subMesh)
//    * \param eventType - ALGO_EVENT or COMPUTE_EVENT (of SMESH_subMesh)
//    * \param subMesh - the submesh where the event occurs
//    */
//   void ProcessEvent(const int event, const int eventType, SMESH_subMesh* subMesh,
//                     EventListenerData*, const SMESH_Hypothesis*)
//   {
//     if ( eventType == SMESH_subMesh::ALGO_EVENT) // all algo events
//     {
//       subMesh->ComputeStateEngine( SMESH_subMesh::MODIF_ALGO_STATE );
//     }
//   }
// }; // struct VertexEventListener

//=============================================================================
/*!
 * \brief Sets event listener to vertex submeshes
 * \param subMesh - submesh where algo is set
 *
 * This method is called when a submesh gets HYP_OK algo_state.
 * After being set, event listener is notified on each event of a submesh.
 */
//=============================================================================

void StdMeshers_Regular_1D::SetEventListener(SMESH_subMesh* subMesh)
{
  StdMeshers_Propagation::SetPropagationMgr( subMesh );
}

//=============================================================================
/*!
 * \brief Do nothing
 * \param subMesh - restored submesh
 *
 * This method is called only if a submesh has HYP_OK algo_state.
 */
//=============================================================================

void StdMeshers_Regular_1D::SubmeshRestored(SMESH_subMesh* /*subMesh*/)
{
}

//=============================================================================
/*!
 * \brief Return StdMeshers_SegmentLengthAroundVertex assigned to vertex
 */
//=============================================================================

const StdMeshers_SegmentLengthAroundVertex*
StdMeshers_Regular_1D::getVertexHyp(SMESH_Mesh &          theMesh,
                                    const TopoDS_Vertex & theV)
{
  static SMESH_HypoFilter filter( SMESH_HypoFilter::HasName("SegmentAroundVertex_0D"));
  if ( const SMESH_Hypothesis * h = theMesh.GetHypothesis( theV, filter, true ))
  {
    SMESH_Algo* algo = const_cast< SMESH_Algo* >( static_cast< const SMESH_Algo* > ( h ));
    const list <const SMESHDS_Hypothesis *> & hypList = algo->GetUsedHypothesis( theMesh, theV, 0 );
    if ( !hypList.empty() && string("SegmentLengthAroundVertex") == hypList.front()->GetName() )
      return static_cast<const StdMeshers_SegmentLengthAroundVertex*>( hypList.front() );
  }
  return 0;
}

//================================================================================
/*!
 * \brief Divide a curve into equal segments
 */
//================================================================================

bool StdMeshers_Regular_1D::divideIntoEqualSegments( SMESH_Mesh &        theMesh,
                                                     Adaptor3d_Curve &   theC3d,
                                                     smIdType            theNbPoints,
                                                     double              theTol,
                                                     double              theLength,
                                                     double              theFirstU,
                                                     double              theLastU,
                                                     std::list<double> & theParameters )
{
  bool ok = false;
  if ( theNbPoints < IntegerLast() )
  {
    int nbPnt = FromSmIdType<int>( theNbPoints );
    GCPnts_UniformAbscissa discret(theC3d, nbPnt, theFirstU, theLastU, theTol );
    if ( !discret.IsDone() )
      return error( "GCPnts_UniformAbscissa failed");
    if ( discret.NbPoints() < nbPnt )
      discret.Initialize(theC3d, nbPnt + 1, theFirstU, theLastU, theTol );

    int nbPoints = Min( discret.NbPoints(), nbPnt );
    for ( int i = 2; i < nbPoints; i++ ) // skip 1st and last points
    {
      double param = discret.Parameter(i);
      theParameters.push_back( param );
    }
    ok = true;
  }
  else // huge nb segments
  {
    // use FIXED_POINTS_1D method
    StdMeshers_FixedPoints1D fixedPointsHyp( GetGen()->GetANewId(), GetGen() );
    _fpHyp = &fixedPointsHyp;
    std::vector<double>   params = { 0., 1. };
    std::vector<smIdType> nbSegs = { theNbPoints - 1 };
    fixedPointsHyp.SetPoints( params );
    fixedPointsHyp.SetNbSegments( nbSegs );

    HypothesisType curType = _hypType;
    _hypType = FIXED_POINTS_1D;

    ok = computeInternalParameters( theMesh, theC3d, theLength, theFirstU, theLastU,
                                    theParameters, /*reverse=*/false );
    _hypType = curType;
    _fpHyp = 0;
  }
  return ok;
}

//================================================================================
/*!
 * \brief Tune parameters to fit "SegmentLengthAroundVertex" hypothesis
 *  \param theC3d - wire curve
 *  \param theLength - curve length
 *  \param theParameters - internal nodes parameters to modify
 *  \param theVf - 1st vertex
 *  \param theVl - 2nd vertex
 */
//================================================================================

void StdMeshers_Regular_1D::redistributeNearVertices (SMESH_Mesh &          theMesh,
                                                      Adaptor3d_Curve &     theC3d,
                                                      double                theLength,
                                                      std::list< double > & theParameters,
                                                      const TopoDS_Vertex & theVf,
                                                      const TopoDS_Vertex & theVl)
{
  double f = theC3d.FirstParameter(), l = theC3d.LastParameter();
  size_t nPar = theParameters.size();
  for ( int isEnd1 = 0; isEnd1 < 2; ++isEnd1 )
  {
    const TopoDS_Vertex & V = isEnd1 ? theVf : theVl;
    const StdMeshers_SegmentLengthAroundVertex* hyp = getVertexHyp (theMesh, V );
    if ( hyp ) {
      double vertexLength = hyp->GetLength();
      if ( vertexLength > theLength / 2.0 )
        continue;
      if ( isEnd1 ) { // to have a segment of interest at end of theParameters
        theParameters.reverse();
        std::swap( f, l );
      }
      if ( _hypType == NB_SEGMENTS )
      {
        compensateError(0, vertexLength, f, l, theLength, theC3d, theParameters, true );
      }
      else if ( nPar <= 3 )
      {
        if ( !isEnd1 )
          vertexLength = -vertexLength;
        double tol = Min( Precision::Confusion(), 0.01 * vertexLength );
        GCPnts_AbscissaPoint Discret( tol, theC3d, vertexLength, l );
        if ( Discret.IsDone() ) {
          if ( nPar == 0 )
            theParameters.push_back( Discret.Parameter());
          else {
            double L = GCPnts_AbscissaPoint::Length( theC3d, theParameters.back(), l);
            if ( vertexLength < L / 2.0 )
              theParameters.push_back( Discret.Parameter());
            else
              compensateError(0, vertexLength, f, l, theLength, theC3d, theParameters, true );
          }
        }
      }
      else
      {
        // recompute params between the last segment and a middle one.
        // find size of a middle segment
        smIdType nHalf = ( nPar-1 ) / 2;
        list< double >::reverse_iterator itU = theParameters.rbegin();
        std::advance( itU, nHalf );
        double Um = *itU++;
        double Lm = GCPnts_AbscissaPoint::Length( theC3d, Um, *itU);
        double L = GCPnts_AbscissaPoint::Length( theC3d, *itU, l);
        static StdMeshers_Regular_1D* auxAlgo = 0;
        if ( !auxAlgo ) {
          auxAlgo = new StdMeshers_Regular_1D( _gen->GetANewId(), _gen );
          auxAlgo->_hypType = BEG_END_LENGTH;
        }
        auxAlgo->_value[ BEG_LENGTH_IND ] = Lm;
        auxAlgo->_value[ END_LENGTH_IND ] = vertexLength;
        double from = *itU, to = l;
        if ( isEnd1 ) {
          std::swap( from, to );
          std::swap( auxAlgo->_value[ BEG_LENGTH_IND ], auxAlgo->_value[ END_LENGTH_IND ]);
        }
        list<double> params;
        if ( auxAlgo->computeInternalParameters( theMesh, theC3d, L, from, to, params, false ))
        {
          if ( isEnd1 ) params.reverse();
          while ( 1 + nHalf-- )
            theParameters.pop_back();
          theParameters.splice( theParameters.end(), params );
        }
        else
        {
          compensateError(0, vertexLength, f, l, theLength, theC3d, theParameters, true );
        }
      }
      if ( isEnd1 )
        theParameters.reverse();
    }
  }
}

bool StdMeshers_Regular_1D::computeBetaLaw(
  Adaptor3d_Curve& theC3d,
  std::list<double>& theParams,
  double f,
  double theLength,
  double beta,
  int nbSegments,
  bool theReverse
  )
{
  // Implemented with formula, where h is the position of a point on the segment [0,1]:
  // ratio=(1+beta)/(beta -1)
  // zlog=log(ratio)
  // puiss=exp(zlog*(1-h))
  // rapp=(1-puiss)/(1+puiss)
  // f(h) =1+beta*rapp
  //
  // Look at https://gitlab.onelab.info/gmsh/gmsh/-/commit/d581b381f2b8639fba40f2e771e2573d1a0f8424
  // Especially gmsh/src/mesh/meshGEdge.cpp, 507: createPoints()

  if (theReverse)
  {
    beta *= -1;
  }

  MESSAGE("Compute BetaLaw. beta: " << beta);

  // Prepare a temp storage for position values
  const int nbNewPoints = nbSegments - 1;
  std::vector<double> t(nbNewPoints);

  // Calculate position values with beta for each point
  const double zlog = log((1. + beta) / (beta - 1.));
  for(smIdType i = 0; i < nbNewPoints; i++)
  {
    const double eta = (double)(i + 1) / nbSegments;
    const double power = exp(zlog * (1. - eta));
    const double ratio = (1. - power) / (1. + power);
    const double pos = 1.0 + beta * ratio;

    // Check if we need to reverse distribution
    if (beta > 0)
    {
      t[i] = pos;
    }
    else
    {
      t[nbNewPoints - i - 1] = 1.0 - pos;
    }

    // Commented to prevent bloated output with a casual debug
    // MESSAGE("Calculated position " << i << ": " << pos);
  }

  // Make points for each calculated value
  for(const auto i : t)
  {
    const double abscissa = i * theLength;
    MESSAGE("abscissa: " << abscissa);

    GCPnts_AbscissaPoint Discret(Precision::Confusion(), theC3d, abscissa, f);
    if (Discret.IsDone())
      theParams.push_back(Discret.Parameter());
  }

  return true;
}

//=============================================================================
/*!
 *
 */
//=============================================================================
bool StdMeshers_Regular_1D::computeInternalParameters(SMESH_Mesh &     theMesh,
                                                      Adaptor3d_Curve& theC3d,
                                                      double           theLength,
                                                      double           theFirstU,
                                                      double           theLastU,
                                                      list<double> &   theParams,
                                                      const bool       theReverse,
                                                      bool             theConsiderPropagation)
{
  theParams.clear();

  double f = theFirstU, l = theLastU;

  // Propagation Of Distribution
  //
  if ( !_mainEdge.IsNull() && _hypType == DISTRIB_PROPAGATION )
  {
    TopoDS_Edge mainEdge = TopoDS::Edge( _mainEdge ); // should not be a reference!
    _gen->Compute( theMesh, mainEdge, SMESH_Gen::SHAPE_ONLY_UPWARD );

    SMESHDS_SubMesh* smDS = theMesh.GetMeshDS()->MeshElements( mainEdge );
    if ( !smDS )
      return error("No mesh on the source edge of Propagation Of Distribution");
    if ( smDS->NbNodes() < 1 )
      return true; // 1 segment

    map< double, const SMDS_MeshNode* > mainEdgeParamsOfNodes;
    if ( ! SMESH_Algo::GetSortedNodesOnEdge( theMesh.GetMeshDS(), mainEdge, _quadraticMesh,
                                             mainEdgeParamsOfNodes, SMDSAbs_Edge ))
      return error("Bad node parameters on the source edge of Propagation Of Distribution");
    vector< double > segLen( mainEdgeParamsOfNodes.size() - 1 );
    double totalLen = 0;
    BRepAdaptor_Curve mainEdgeCurve( mainEdge );
    map< double, const SMDS_MeshNode* >::iterator
      u_n2 = mainEdgeParamsOfNodes.begin(), u_n1 = u_n2++;
    for ( size_t i = 1; i < mainEdgeParamsOfNodes.size(); ++i, ++u_n1, ++u_n2 )
    {
      segLen[ i-1 ] = GCPnts_AbscissaPoint::Length( mainEdgeCurve,
                                                    u_n1->first,
                                                    u_n2->first);
      totalLen += segLen[ i-1 ];
    }
    for ( size_t i = 0; i < segLen.size(); ++i )
      segLen[ i ] *= theLength / totalLen;

    size_t  iSeg = theReverse ? segLen.size()-1 : 0;
    size_t  dSeg = theReverse ? -1 : +1;
    double param = theFirstU;
    size_t nbParams = 0;
    for ( size_t i = 1; i < segLen.size(); ++i, iSeg += dSeg )
    {
      double tol = Min( Precision::Confusion(), 0.01 * segLen[ iSeg ]);
      GCPnts_AbscissaPoint Discret( tol, theC3d, segLen[ iSeg ], param );
      if ( !Discret.IsDone() ) break;
      param = Discret.Parameter();
      theParams.push_back( param );
      ++nbParams;
    }
    if ( nbParams != segLen.size()-1 )
      return error( SMESH_Comment("Can't divide into ") << segLen.size() << " segments");

    compensateError( segLen[ theReverse ? segLen.size()-1 : 0 ],
                     segLen[ theReverse ? 0 : segLen.size()-1 ],
                     f, l, theLength, theC3d, theParams, true );
    return true;
  }


  switch( _hypType )
  {
  case LOCAL_LENGTH:
  case MAX_LENGTH:
  case NB_SEGMENTS:
  {
    double eltSize = 1;
    smIdType nbSegments;
    if ( _hypType == MAX_LENGTH )
    {
      double nbseg = ceil(theLength / _value[ BEG_LENGTH_IND ]); // integer sup
      if (nbseg <= 0)
        nbseg = 1; // degenerated edge
      eltSize = theLength / nbseg * ( 1. - 1e-9 );
      nbSegments = ToSmIdType( nbseg );
    }
    else if ( _hypType == LOCAL_LENGTH )
    {
      // Local Length hypothesis
      double nbseg = ceil(theLength / _value[ BEG_LENGTH_IND ]); // integer sup

      // NPAL17873:
      bool isFound = false;
      if (theConsiderPropagation && !_mainEdge.IsNull()) // propagated from some other edge
      {
        // Advanced processing to assure equal number of segments in case of Propagation
        SMESH_subMesh* sm = theMesh.GetSubMeshContaining(_mainEdge);
        if (sm) {
          bool computed = sm->IsMeshComputed();
          if (!computed) {
            if (sm->GetComputeState() == SMESH_subMesh::READY_TO_COMPUTE) {
              _gen->Compute( theMesh, _mainEdge, /*anUpward=*/true);
              computed = sm->IsMeshComputed();
            }
          }
          if (computed) {
            SMESHDS_SubMesh* smds = sm->GetSubMeshDS();
            smIdType  nb_segments = smds->NbElements();
            if (nbseg - 1 <= nb_segments && nb_segments <= nbseg + 1) {
              isFound = true;
              nbseg = FromSmIdType<double>( nb_segments );
            }
          }
        }
      }
      if (!isFound) // not found by meshed edge in the propagation chain, use precision
      {
        double aPrecision = _value[ PRECISION_IND ];
        double nbseg_prec = ceil((theLength / _value[ BEG_LENGTH_IND ]) - aPrecision);
        if (nbseg_prec == (nbseg - 1)) nbseg--;
      }

      if (nbseg <= 0)
        nbseg = 1;                        // degenerated edge
      eltSize = theLength / nbseg;
      nbSegments = ToSmIdType( nbseg );
    }
    else
    {
      // Number Of Segments hypothesis
      nbSegments = _ivalue[ NB_SEGMENTS_IND ];
      if ( nbSegments < 1 )  return false;
      if ( nbSegments == 1 ) return true;

      switch (_ivalue[ DISTR_TYPE_IND ])
      {
      case StdMeshers_NumberOfSegments::DT_Scale:
        {
          double scale = _value[ SCALE_FACTOR_IND ];

          if (fabs(scale - 1.0) < Precision::Confusion()) {
            // special case to avoid division by zero
            for ( smIdType i = 1; i < nbSegments; i++) {
              double param = f + (l - f) * double( i ) / double( nbSegments );
              theParams.push_back( param );
            }
          }
          else { // general case of scale distribution
            if ( theReverse )
              scale = 1.0 / scale;

            double  alpha = pow(scale, 1.0 / double( nbSegments - 1 ));
            double factor = (l - f) / (1.0 - pow(alpha, nbSegments));

            for ( smIdType i = 1; i < nbSegments; i++) {
              double param = f + factor * (1.0 - pow(alpha, i));
              theParams.push_back( param );
            }
          }
          const double lenFactor = theLength/(l-f);
          const double minSegLen = Min( theParams.front() - f, l - theParams.back() );
          const double       tol = Min( Precision::Confusion(), 0.01 * minSegLen );
          list<double>::iterator u = theParams.begin(), uEnd = theParams.end();
          for ( ; u != uEnd; ++u )
          {
            GCPnts_AbscissaPoint Discret( tol, theC3d, ((*u)-f) * lenFactor, f );
            if ( Discret.IsDone() )
              *u = Discret.Parameter();
          }
          return true;
        }
        break;
      case StdMeshers_NumberOfSegments::DT_TabFunc:
        {
          FunctionTable func(_vvalue[ TAB_FUNC_IND ], FromSmIdType<int>( _ivalue[ CONV_MODE_IND ]));
          return computeParamByFunc(theC3d, f, l, theLength, theReverse,
                                    _ivalue[ NB_SEGMENTS_IND ], func,
                                    theParams);
        }
        break;
      case StdMeshers_NumberOfSegments::DT_ExprFunc:
        {
          FunctionExpr func(_svalue[ EXPR_FUNC_IND ].c_str(),
                            FromSmIdType<int>( _ivalue[ CONV_MODE_IND ]));
          return computeParamByFunc(theC3d, f, l, theLength, theReverse,
                                    _ivalue[ NB_SEGMENTS_IND ], func,
                                    theParams);
        }
        break;

      case StdMeshers_NumberOfSegments::DT_BetaLaw:
        return computeBetaLaw(theC3d, theParams, f, theLength, _value[BETA_IND], nbSegments, theReverse);

      case StdMeshers_NumberOfSegments::DT_Regular:
        eltSize = theLength / double( nbSegments );
        break;
      default:
        return false;
      }
    }

    double tol = Min( Precision::Confusion(), 0.01 * eltSize );
    divideIntoEqualSegments( theMesh, theC3d, nbSegments + 1, tol,
                             theLength, theFirstU, theLastU, theParams );

    compensateError( eltSize, eltSize, f, l, theLength, theC3d, theParams, true ); // for PAL9899
    return true;
  }


  case BEG_END_LENGTH: {

    // geometric progression: SUM(n) = ( a1 - an * q ) / ( 1 - q ) = theLength

    double a1 = _value[ BEG_LENGTH_IND ];
    double an = _value[ END_LENGTH_IND ];
    double q  = ( theLength - a1 ) / ( theLength - an );
    if ( q < theLength/1e6 || 1.01*theLength < a1 + an)
      return error ( SMESH_Comment("Invalid segment lengths (")<<a1<<" and "<<an<<") "<<
                     "for an edge of length "<<theLength);

    double      U1 = theReverse ? l : f;
    double      Un = theReverse ? f : l;
    double   param = U1;
    double eltSize = theReverse ? -a1 : a1;
    double     tol = Min( Precision::Confusion(), 0.01 * Min( a1, an ));
    while ( 1 ) {
      // computes a point on a curve <theC3d> at the distance <eltSize>
      // from the point of parameter <param>.
      GCPnts_AbscissaPoint Discret( tol, theC3d, eltSize, param );
      if ( !Discret.IsDone() ) break;
      param = Discret.Parameter();
      if ( f < param && param < l )
        theParams.push_back( param );
      else
        break;
      eltSize *= q;
    }
    compensateError( a1, an, U1, Un, theLength, theC3d, theParams );
    if (theReverse) theParams.reverse(); // NPAL18025
    return true;
  }

  case ARITHMETIC_1D:
  {
    // arithmetic progression: SUM(n) = ( an - a1 + q ) * ( a1 + an ) / ( 2 * q ) = theLength

    double a1 = _value[ BEG_LENGTH_IND ];
    double an = _value[ END_LENGTH_IND ];
    if ( 1.01*theLength < a1 + an )
      return error ( SMESH_Comment("Invalid segment lengths (")<<a1<<" and "<<an<<") "<<
                     "for an edge of length "<<theLength);

    // Compute first the number of segments and then the arithmetic increment based on that number
    int    n = static_cast<int>(2 * theLength / ( a1 + an ) + 0.5);
    double q = (n > 1 ? ( an - a1 ) / (n - 1) : 0.0);

    double      U1 = theReverse ? l : f;
    double      Un = theReverse ? f : l;
    double   param = U1;
    double eltSize = a1;
    double     tol = Min( Precision::Confusion(), 0.01 * Min( a1, an ));
    if ( theReverse ) {
      eltSize = -eltSize;
      q = -q;
    }
    for (int i=0; i<n; i++) {
      // computes a point on a curve <theC3d> at the distance <eltSize>
      // from the point of parameter <param>.
      GCPnts_AbscissaPoint Discret( tol, theC3d, eltSize, param );
      if ( !Discret.IsDone() ) break;
      param = Discret.Parameter();
      theParams.push_back( param );
      eltSize += q;
    }

    distributeError( a1, an, U1, Un, theLength, theC3d, theParams );

    // Do not include the parameter for the start or end of an edge in the list of parameters
    // NOTE: it is required to correctly distribute the error
    if (fabs(theParams.front() - U1) < tol) theParams.pop_front();
    if (fabs(theParams.back() - Un) < tol)  theParams.pop_back();

    if ( theReverse ) theParams.reverse(); // NPAL18025

    return true;
  }

  case GEOMETRIC_1D:
  {
    double a1 = _value[ BEG_LENGTH_IND ], an = 0;
    double q  = _value[ END_LENGTH_IND ];

    double U1 = theReverse ? l : f;
    double Un = theReverse ? f : l;
    double param = U1;
    double eltSize = a1;
    if ( theReverse )
      eltSize = -eltSize;

    int nbParams = 0;
    while ( true ) {
      // computes a point on a curve <theC3d> at the distance <eltSize>
      // from the point of parameter <param>.
      double tol = Min( Precision::Confusion(), 0.01 * eltSize );
      GCPnts_AbscissaPoint Discret( tol, theC3d, eltSize, param );
      if ( !Discret.IsDone() ) break;
      param = Discret.Parameter();
      if ( f < param && param < l )
        theParams.push_back( param );
      else
        break;
      an = eltSize;
      eltSize *= q;
      ++nbParams;
      if ( q < 1. && eltSize < 1e-100 )
        return error("Too small common ratio causes too many segments");
    }
    if ( nbParams > 1 )
    {
      if ( Abs( param - Un ) < 0.2 * Abs( param - theParams.back() ))
      {
        compensateError( a1, Abs(eltSize), U1, Un, theLength, theC3d, theParams );
      }
      else if ( Abs( Un - theParams.back() ) <
                0.2 * Abs( theParams.back() - *(++theParams.rbegin())))
      {
        theParams.pop_back();
        compensateError( a1, Abs(an), U1, Un, theLength, theC3d, theParams );
      }
    }
    if (theReverse) theParams.reverse(); // NPAL18025

    return true;
  }

  case FIXED_POINTS_1D:
  {
    const std::vector<double>& aPnts = _fpHyp->GetPoints();
    std::vector<smIdType>     nbsegs = _fpHyp->GetNbSegments();

    // sort normalized params, taking into account theReverse
    TColStd_SequenceOfReal Params;
    double tol = 1e-7;
    for ( size_t i = 0; i < aPnts.size(); i++ )
    {
      if( aPnts[i] < tol || aPnts[i] > 1 - tol )
        continue;
      double u = theReverse ? ( 1 - aPnts[i] ) : aPnts[i];
      int    j = 1;
      bool IsExist = false;
      for ( ; j <= Params.Length() &&  !IsExist; j++ )
      {
        IsExist = ( Abs( u - Params.Value(j) ) < tol );
        if ( u < Params.Value(j) ) break;
      }
      if ( !IsExist ) Params.InsertBefore( j, u );
    }
    Params.InsertBefore( 1, 0.0 );
    Params.Append( 1.0 );

    if ( theReverse )
    {
      if ((int) nbsegs.size() > Params.Length() - 1 )
        nbsegs.resize( Params.Length() - 1 );
      std::reverse( nbsegs.begin(), nbsegs.end() );
    }
    if ( nbsegs.empty() )
    {
      nbsegs.push_back( 1 );
    }
    if ((int) nbsegs.size() < Params.Length() - 1 )
      nbsegs.resize( Params.Length() - 1, nbsegs[0] );

    // care of huge nbsegs - additionally divide diapasons
    for ( int i = 2; i <= Params.Length(); i++ )
    {
      smIdType nbTot = nbsegs[ i-2 ];
      if ( nbTot <= IntegerLast() )
        continue;
      smIdType   nbDiapason = nbTot / IntegerLast() + 1;
      smIdType nbSegPerDiap = nbTot / nbDiapason;
      double           par0 = Params( i - 1 ), par1 = Params( i );
      for ( smIdType iDiap = 0; iDiap < nbDiapason - 1; ++iDiap )
      {
        double    r = double( nbSegPerDiap * ( iDiap + 1 )) / double( nbTot );
        double parI = par0 + ( par1 - par0 ) * r;
        Params.InsertBefore( i, parI );
        auto it = nbsegs.begin();
        smIdType incr_it = i - 2 + iDiap;
        nbsegs.insert( it + incr_it, nbSegPerDiap );
      }
      nbsegs[ i-2 + nbDiapason - 1 ] = nbSegPerDiap + nbTot % nbDiapason;
    }

    // transform normalized Params into real ones
    std::vector< double > uVec( Params.Length() );
    uVec[ 0 ] = theFirstU;
    double abscissa;
    for ( int i = 2; i < Params.Length(); i++ )
    {
      abscissa = Params( i ) * theLength;
      tol      = Min( Precision::Confusion(), 0.01 * abscissa );
      GCPnts_AbscissaPoint APnt( tol, theC3d, abscissa, theFirstU );
      if ( !APnt.IsDone() )
        return error( "GCPnts_AbscissaPoint failed");
      uVec[ i-1 ] = APnt.Parameter();
    }
    uVec.back() = theLastU;

    // divide segments
    double eltSize, segmentSize, par1, par2;
    for ( int i = 0; i < (int)uVec.size()-1; i++ )
    {
      par1 = uVec[ i   ];
      par2 = uVec[ i+1 ];
      smIdType nbseg = ( i < (int) nbsegs.size() ) ? nbsegs[i] : nbsegs[0];
      if ( nbseg > 1 )
      {
        segmentSize = ( Params( i+2 ) - Params( i+1 )) * theLength;
        eltSize     = segmentSize / double( nbseg );
        tol         = Min( Precision::Confusion(), 0.01 * eltSize );
        if ( !divideIntoEqualSegments( theMesh, theC3d, nbseg + 1, tol,
                                       segmentSize, par1, par2, theParams ))
          return false;
      }
      theParams.push_back( par2 );
    }
    theParams.pop_back();

    return true;
  }

  case DEFLECTION:
  {
    GCPnts_UniformDeflection Discret( theC3d, _value[ DEFLECTION_IND ], f, l, true );
    if ( !Discret.IsDone() )
      return false;

    int NbPoints = Discret.NbPoints();
    for ( int i = 2; i < NbPoints; i++ )
    {
      double param = Discret.Parameter(i);
      theParams.push_back( param );
    }
    return true;
  }

  default:;
  }

  return false;
}

//=============================================================================
/*!
 *
 */
//=============================================================================

bool StdMeshers_Regular_1D::Compute(SMESH_Mesh & theMesh, const TopoDS_Shape & theShape)
{
  if ( _hypType == NONE )
    return false;

  if ( _hypType == ADAPTIVE )
  {
    _adaptiveHyp->GetAlgo()->InitComputeError();
    _adaptiveHyp->GetAlgo()->Compute( theMesh, theShape );
    return error( _adaptiveHyp->GetAlgo()->GetComputeError() );
  }

  SMESHDS_Mesh * meshDS = theMesh.GetMeshDS();

  const TopoDS_Edge & EE = TopoDS::Edge(theShape);
  TopoDS_Edge E = TopoDS::Edge(EE.Oriented(TopAbs_FORWARD));
  int shapeID = meshDS->ShapeToIndex( E );

  double f, l;
  Handle(Geom_Curve) Curve = BRep_Tool::Curve(E, f, l);

  TopoDS_Vertex VFirst, VLast;
  TopExp::Vertices(E, VFirst, VLast);   // Vfirst corresponds to f and Vlast to l

  ASSERT(!VFirst.IsNull());
  ASSERT(!VLast.IsNull());
  const SMDS_MeshNode * nFirst = SMESH_Algo::VertexNode( VFirst, meshDS );
  const SMDS_MeshNode *  nLast = SMESH_Algo::VertexNode( VLast,  meshDS );
  if ( !nFirst || !nLast )
    return error( COMPERR_BAD_INPUT_MESH, "No node on vertex");

  // remove elements created by e.g. pattern mapping (PAL21999)
  // CLEAN event is incorrectly ptopagated seemingly due to Propagation hyp
  // so TEMPORARY solution is to clean the submesh manually
  if (SMESHDS_SubMesh * subMeshDS = meshDS->MeshElements(theShape))
  {
    SMDS_ElemIteratorPtr ite = subMeshDS->GetElements();
    while (ite->more())
      meshDS->RemoveFreeElement(ite->next(), subMeshDS);
    SMDS_NodeIteratorPtr itn = subMeshDS->GetNodes();
    while (itn->more()) {
      const SMDS_MeshNode * node = itn->next();
      if ( node->NbInverseElements() == 0 )
        meshDS->RemoveFreeNode(node, subMeshDS);
      else
        meshDS->RemoveNode(node);
    }
  }

  double length = EdgeLength( E );
  if ( !Curve.IsNull() && length > 0 )
  {
    list< double > params;
    bool reversed = false;
    if ( theMesh.GetShapeToMesh().ShapeType() >= TopAbs_WIRE && _revEdgesIDs.empty() ) {
      // if the shape to mesh is WIRE or EDGE
      reversed = ( EE.Orientation() == TopAbs_REVERSED );
    }
    if ( !_mainEdge.IsNull() ) {
      // take into account reversing the edge the hypothesis is propagated from
      // (_mainEdge.Orientation() marks mutual orientation of EDGEs in propagation chain)
      reversed = ( _mainEdge.Orientation() == TopAbs_REVERSED );
      if ( _hypType != DISTRIB_PROPAGATION ) {
        int mainID = meshDS->ShapeToIndex(_mainEdge);
        if ( std::find( _revEdgesIDs.begin(), _revEdgesIDs.end(), mainID) != _revEdgesIDs.end())
          reversed = !reversed;
      }
    }
    // take into account this edge reversing
    if ( std::find( _revEdgesIDs.begin(), _revEdgesIDs.end(), shapeID) != _revEdgesIDs.end())
      reversed = !reversed;

    BRepAdaptor_Curve C3d( E );
    if ( ! computeInternalParameters( theMesh, C3d, length, f, l, params, reversed, true )) {
      return false;
    }
    redistributeNearVertices( theMesh, C3d, length, params, VFirst, VLast );

    // edge extrema (indexes : 1 & NbPoints) already in SMDS (TopoDS_Vertex)
    // only internal nodes receive an edge position with param on curve

    const SMDS_MeshNode * nPrev = nFirst;
    double parPrev = f;
    double parLast = l;

    for (list<double>::iterator itU = params.begin(); itU != params.end(); itU++) {
      double param = *itU;
      gp_Pnt P = Curve->Value(param);

      //Add the Node in the DataStructure
      SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnEdge(node, shapeID, param);

      if(_quadraticMesh) {
        // create medium node
        double prm = ( parPrev + param )/2;
        gp_Pnt  PM = Curve->Value(prm);
        SMDS_MeshNode * NM = meshDS->AddNode(PM.X(), PM.Y(), PM.Z());
        meshDS->SetNodeOnEdge(NM, shapeID, prm);
        SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, node, NM);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }
      else {
        SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, node);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }

      nPrev   = node;
      parPrev = param;
    }
    if(_quadraticMesh) {
      double prm = ( parPrev + parLast )/2;
      gp_Pnt PM = Curve->Value(prm);
      SMDS_MeshNode * NM = meshDS->AddNode(PM.X(), PM.Y(), PM.Z());
      meshDS->SetNodeOnEdge(NM, shapeID, prm);
      SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, nLast, NM);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
    else {
      SMDS_MeshEdge* edge = meshDS->AddEdge(nPrev, nLast);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
  }
  else
  {
    // Edge is a degenerated Edge : We put n = 5 points on the edge.
    const int NbPoints = 5;
    BRep_Tool::Range( E, f, l ); // PAL15185
    double du = (l - f) / (NbPoints - 1);

    gp_Pnt P = BRep_Tool::Pnt(VFirst);

    const SMDS_MeshNode * nPrev = nFirst;
    for (int i = 2; i < NbPoints; i++) {
      double param = f + (i - 1) * du;
      SMDS_MeshNode * node = meshDS->AddNode(P.X(), P.Y(), P.Z());
      if(_quadraticMesh) {
        // create medium node
        double prm = param - du/2.;
        SMDS_MeshNode * NM = meshDS->AddNode(P.X(), P.Y(), P.Z());
        meshDS->SetNodeOnEdge(NM, shapeID, prm);
        SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, node, NM);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }
      else {
        SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, node);
        meshDS->SetMeshElementOnShape(edge, shapeID);
      }
      meshDS->SetNodeOnEdge(node, shapeID, param);
      nPrev = node;
    }
    if(_quadraticMesh) {
      // create medium node
      double prm = l - du/2.;
      SMDS_MeshNode * NM = meshDS->AddNode(P.X(), P.Y(), P.Z());
      meshDS->SetNodeOnEdge(NM, shapeID, prm);
      SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, nLast, NM);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
    else {
      SMDS_MeshEdge * edge = meshDS->AddEdge(nPrev, nLast);
      meshDS->SetMeshElementOnShape(edge, shapeID);
    }
  }
  return true;
}


//=============================================================================
/*!
 *
 */
//=============================================================================

bool StdMeshers_Regular_1D::Evaluate(SMESH_Mesh &         theMesh,
                                     const TopoDS_Shape & theShape,
                                     MapShapeNbElems&     theResMap)
{
  if ( _hypType == NONE )
    return false;

  if ( _hypType == ADAPTIVE )
  {
    _adaptiveHyp->GetAlgo()->InitComputeError();
    _adaptiveHyp->GetAlgo()->Evaluate( theMesh, theShape, theResMap );
    return error( _adaptiveHyp->GetAlgo()->GetComputeError() );
  }

  const TopoDS_Edge & EE = TopoDS::Edge(theShape);
  TopoDS_Edge E = TopoDS::Edge(EE.Oriented(TopAbs_FORWARD));

  double f, l;
  Handle(Geom_Curve) Curve = BRep_Tool::Curve(E, f, l);

  TopoDS_Vertex VFirst, VLast;
  TopExp::Vertices(E, VFirst, VLast);   // Vfirst corresponds to f and Vlast to l

  ASSERT(!VFirst.IsNull());
  ASSERT(!VLast.IsNull());

  std::vector<smIdType> aVec(SMDSEntity_Last,0);

  double length = EdgeLength( E );
  if ( !Curve.IsNull() && length > 0 )
  {
    list< double > params;
    BRepAdaptor_Curve C3d( E );
    if ( ! computeInternalParameters( theMesh, C3d, length, f, l, params, false, true )) {
      SMESH_subMesh * sm = theMesh.GetSubMesh(theShape);
      theResMap.insert(std::make_pair(sm,aVec));
      SMESH_ComputeErrorPtr& smError = sm->GetComputeError();
      smError.reset( new SMESH_ComputeError(COMPERR_ALGO_FAILED,"Submesh can not be evaluated",this));
      return false;
    }
    redistributeNearVertices( theMesh, C3d, length, params, VFirst, VLast );

    if(_quadraticMesh) {
      aVec[SMDSEntity_Node     ] = 2*params.size() + 1;
      aVec[SMDSEntity_Quad_Edge] = params.size() + 1;
    }
    else {
      aVec[SMDSEntity_Node] = params.size();
      aVec[SMDSEntity_Edge] = params.size() + 1;
    }

  }
  else {
    // Edge is a degenerated Edge : We put n = 5 points on the edge.
    if ( _quadraticMesh ) {
      aVec[SMDSEntity_Node     ] = 11;
      aVec[SMDSEntity_Quad_Edge] = 6;
    }
    else {
      aVec[SMDSEntity_Node] = 5;
      aVec[SMDSEntity_Edge] = 6;
    }
  }

  SMESH_subMesh * sm = theMesh.GetSubMesh( theShape );
  theResMap.insert( std::make_pair( sm, aVec ));

  return true;
}


//=============================================================================
/*!
 *  See comments in SMESH_Algo.cxx
 */
//=============================================================================

const list <const SMESHDS_Hypothesis *> &
StdMeshers_Regular_1D::GetUsedHypothesis(SMESH_Mesh &         aMesh,
                                         const TopoDS_Shape & aShape,
                                         const bool           ignoreAuxiliary)
{
  _usedHypList.clear();
  _mainEdge.Nullify();

  SMESH_HypoFilter auxiliaryFilter( SMESH_HypoFilter::IsAuxiliary() );
  const SMESH_HypoFilter* compatibleFilter = GetCompatibleHypoFilter(/*ignoreAux=*/true );

  // get non-auxiliary assigned directly to aShape
  int nbHyp = aMesh.GetHypotheses( aShape, *compatibleFilter, _usedHypList, false );

  if (nbHyp == 0 && aShape.ShapeType() == TopAbs_EDGE)
  {
    // Check, if propagated from some other edge
    bool isPropagOfDistribution = false;
    _mainEdge = StdMeshers_Propagation::GetPropagationSource( aMesh, aShape,
                                                              isPropagOfDistribution );
    if ( !_mainEdge.IsNull() )
    {
      if ( isPropagOfDistribution )
        _hypType = DISTRIB_PROPAGATION;
      // Propagation of 1D hypothesis from <aMainEdge> on this edge;
      // get non-auxiliary assigned to _mainEdge
      nbHyp = aMesh.GetHypotheses( _mainEdge, *compatibleFilter, _usedHypList, true );
    }
  }

  if (nbHyp == 0) // nothing propagated nor assigned to aShape
  {
    SMESH_Algo::GetUsedHypothesis( aMesh, aShape, ignoreAuxiliary );
    nbHyp = (int)_usedHypList.size();
  }
  else
  {
    // get auxiliary hyps from aShape
    aMesh.GetHypotheses( aShape, auxiliaryFilter, _usedHypList, true );
  }
  if ( nbHyp > 1 && ignoreAuxiliary )
    _usedHypList.clear(); //only one compatible non-auxiliary hypothesis allowed

  return _usedHypList;
}

//================================================================================
/*!
 * \brief Pass CancelCompute() to a child algorithm
 */
//================================================================================

void StdMeshers_Regular_1D::CancelCompute()
{
  SMESH_Algo::CancelCompute();
  if ( _hypType == ADAPTIVE )
    _adaptiveHyp->GetAlgo()->CancelCompute();
}
