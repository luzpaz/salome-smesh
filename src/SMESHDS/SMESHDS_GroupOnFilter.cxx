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

//  File   : SMESHDS_GroupOnFilter.cxx
//  Module : SMESH
//
#include "SMESHDS_GroupOnFilter.hxx"

#include "SMDS_SetIterator.hxx"
#include "ObjectPool.hxx"
#include "SMESHDS_Mesh.hxx"

#include <numeric>
#include <limits>

#include <boost/make_shared.hpp>

using namespace std;

//#undef WITH_TBB

//=============================================================================
/*!
 * Creates a group based on thePredicate
 */
//=============================================================================

SMESHDS_GroupOnFilter::SMESHDS_GroupOnFilter (const int                 theID,
                                              const SMESHDS_Mesh*       theMesh,
                                              const SMDSAbs_ElementType theType,
                                              const SMESH_PredicatePtr& thePredicate)
  : SMESHDS_GroupBase( theID, theMesh, theType ),
    SMDS_ElementHolder( theMesh ),
    myMeshInfo( SMDSEntity_Last, 0 ),
    myMeshModifTime( 0 ),
    myPredicateTic( 0 ),
    myNbElemToSkip( 0 )
{
  SetPredicate( thePredicate );
}

//================================================================================
/*!
 * \brief Sets a new predicate
 */
//================================================================================

void SMESHDS_GroupOnFilter::SetPredicate( const SMESH_PredicatePtr& thePredicate )
{
  myPredicate = thePredicate;
  ++myPredicateTic;
  setChanged();
  if ( myPredicate )
    myPredicate->SetMesh( GetMesh() );
}

//================================================================================
/*!
 * \brief Returns nb of elements
 */
//================================================================================

smIdType SMESHDS_GroupOnFilter::Extent() const
{
  update();
  return std::accumulate( myMeshInfo.begin(), myMeshInfo.end(), 0 );
}

//================================================================================
/*!
 * \brief Checks emptyness
 */
//================================================================================

bool SMESHDS_GroupOnFilter::IsEmpty()
{
  if ( IsUpToDate() )
  {
    return ( Extent() == 0 );
  }
  else // not up-to-date
  {
    setChanged();
    SMDS_ElemIteratorPtr okElemIt = GetElements();
    if ( !okElemIt->more() )
    {
      // no satisfying elements
      setChanged( false );
    }
    else
    {
      return false;
    }
  }
  return true;
}

//================================================================================
/*!
 * \brief Checks if the element belongs to the group
 */
//================================================================================

bool SMESHDS_GroupOnFilter::Contains (const smIdType theID)
{
  return myPredicate && myPredicate->IsSatisfy( theID );
}

//================================================================================
/*!
 * \brief Checks if the element belongs to the group
 */
//================================================================================

bool SMESHDS_GroupOnFilter::Contains (const SMDS_MeshElement* elem)
{
  return myPredicate && myPredicate->IsSatisfy( elem->GetID() );
}

//================================================================================
namespace // Iterator
{
  struct TIterator : public SMDS_ElemIterator
  {
    SMESH_PredicatePtr                myPredicate;
    SMDS_ElemIteratorPtr              myElemIt;
    const SMDS_MeshElement*           myNextElem;
    size_t                            myNbToFind, myNbFound, myTotalNb;
    vector< const SMDS_MeshElement*>& myFoundElems;
    bool &                            myFoundElemsOK;
    bool                              myFoundElemsChecked;

    TIterator( const SMESH_PredicatePtr&         filter,
               SMDS_ElemIteratorPtr&             elems,
               size_t                            nbToFind,
               size_t                            totalNb,
               vector< const SMDS_MeshElement*>& foundElems,
               bool &                            foundElemsOK):
      myPredicate( filter ),
      myElemIt( elems ),
      myNextElem( 0 ),
      myNbToFind( nbToFind ),
      myNbFound( 0 ),
      myTotalNb( totalNb ),
      myFoundElems( foundElems ),
      myFoundElemsOK( foundElemsOK ),
      myFoundElemsChecked( false )
    {
      myFoundElemsOK = false;
      next();
    }
    ~TIterator()
    {
      if ( !myFoundElemsChecked && !myFoundElemsOK )
        clearVector( myFoundElems );
    }
    virtual bool more()
    {
      return myNextElem;
    }
    virtual const SMDS_MeshElement* next()
    {
      const SMDS_MeshElement* res = myNextElem;
      myNbFound += bool( res );
      myNextElem = 0;
      if ( myNbFound < myNbToFind )
      {
        while ( myElemIt->more() && !myNextElem )
        {
          myNextElem = myElemIt->next();
          if ( !myPredicate->IsSatisfy( myNextElem->GetID() ))
            myNextElem = 0;
        }
        if ( myNextElem )
          myFoundElems.push_back( myNextElem );
        else
          keepOrClearElemVec();
      }
      else
      {
        keepOrClearElemVec();
      }
      return res;
    }
    void keepOrClearElemVec()
    {
      if ( myNbFound == myTotalNb )
      {
        myFoundElemsOK = false; // all elems are OK, no need to keep them
      }
      else
      {
        // nb of bytes used for myFoundElems
        size_t vecMemSize = myFoundElems.size() * sizeof( SMDS_MeshElement* ) / sizeof(char);
        size_t aMB = 1024 * 1024;
        if ( vecMemSize < aMB )
        {
          myFoundElemsOK = true; // < 1 MB - do not clear
        }
        else
        {
          int freeRamMB = SMDS_Mesh::CheckMemory( /*doNotRaise=*/true );
          if ( freeRamMB < 0 )
            myFoundElemsOK = true; // hope it's OK
          else
            myFoundElemsOK = ( freeRamMB * aMB > 10 * vecMemSize );
        }
      }
      if ( !myFoundElemsOK )
        clearVector( myFoundElems );

      myFoundElemsChecked = true; // in destructor: not to clearVector() which may already die
    }
  };

  struct TEmptyIterator : public SMDS_ElemIterator
  {
    virtual bool more()                    { return false; }
    virtual const SMDS_MeshElement* next() { return 0; }
  };
}

//================================================================================
/*!
 * \brief Return iterator on all elements
 */
//================================================================================

SMDS_ElemIteratorPtr SMESHDS_GroupOnFilter::GetElements() const
{
  size_t nbToFind = std::numeric_limits<size_t>::max();
  size_t totalNb  = GetMesh()->GetMeshInfo().NbElements( GetType() );

  SMDS_ElemIteratorPtr elemIt; // iterator on all elements to initialize TIterator
  if ( myPredicate )
  {
    myPredicate->SetMesh( GetMesh() ); // hope myPredicate updates self here if necessary

    if ( !IsUpToDate() )
      updateParallel();

    elemIt = GetMesh()->elementsIterator( GetType() );
    if ( IsUpToDate() )
    {
      if ( myElementsOK )
        return SMDS_ElemIteratorPtr( new SMDS_ElementVectorIterator( myElements.begin(),
                                                                     myElements.end() ));
      nbToFind = Extent();
      if ( nbToFind == totalNb )
        return elemIt; // all elements are OK
      for ( size_t i = 0; i < myNbElemToSkip; ++i )
        elemIt->next(); // skip w/o check
    }
  }
  else
  {
    elemIt = SMDS_ElemIteratorPtr( new TEmptyIterator );
  }

  // the iterator fills myElements if all elements are checked
  SMESHDS_GroupOnFilter* me = const_cast<SMESHDS_GroupOnFilter*>( this );
  return SMDS_ElemIteratorPtr
    ( new TIterator( myPredicate, elemIt, nbToFind, totalNb, me->myElements, me->myElementsOK ));
}

//================================================================================
/*!
 * \brief Return info on sub-types of elements
 */
//================================================================================

std::vector< smIdType > SMESHDS_GroupOnFilter::GetMeshInfo() const
{
  update();
  return myMeshInfo;
}

//================================================================================
/*!
 * \brief Fill ids of elements. And return their number.
 *       \a ids must be pre-allocated using nb of elements of type == GetType()
 */
//================================================================================

int SMESHDS_GroupOnFilter::getElementIds( void* ids, size_t idSize ) const
{
  SMESHDS_GroupOnFilter* me = const_cast<SMESHDS_GroupOnFilter*>( this );

  if ( !IsUpToDate() )
    me->setChanged();
    
  char* curID = (char*) ids;
  SMDS_ElemIteratorPtr elIt = GetElements();
  if ( elIt->more() )
  {
    if ( IsUpToDate() )
    {
      for ( ; elIt->more(); curID += idSize )
        (*(smIdType*) curID) = elIt->next()->GetID();
    }
    else
    {
      // find out nb of elements to skip w/o check before the 1st OK element
      const SMDS_MeshElement* firstOkElem = me->setNbElemToSkip( elIt );

      me->myMeshInfo.assign( SMDSEntity_Last, 0 );
      me->myMeshInfo[ firstOkElem->GetEntityType() ]++;

      (*(smIdType*) curID) = firstOkElem->GetID();
      for ( curID += idSize; elIt->more(); curID += idSize )
      {
        const SMDS_MeshElement* e = elIt->next();
        (*(smIdType*) curID) = e->GetID();
        me->myMeshInfo[ e->GetEntityType() ]++;
      }
    }
  }
  me->setChanged( false );

  return ( curID - (char*)ids ) / idSize;
}

//================================================================================
/*!
 * \brief Return a value allowing to find out if a group has changed or not
 */
//================================================================================

int SMESHDS_GroupOnFilter::GetTic() const
{
  return GetMesh()->GetMTime() * myPredicateTic;
}

//================================================================================
/*!
 * \brief Return false if update() is needed
 */
//================================================================================

bool SMESHDS_GroupOnFilter::IsUpToDate() const
{
  return !( myMeshModifTime < GetMesh()->GetMTime() );
}

//================================================================================
/*!
 * \brief Updates myElements if necessary
 */
//================================================================================

void SMESHDS_GroupOnFilter::update() const
{
  SMESHDS_GroupOnFilter* me = const_cast<SMESHDS_GroupOnFilter*>( this );
  if ( !IsUpToDate() )
  {
    me->setChanged();
    if ( !updateParallel() )
    {
      SMDS_ElemIteratorPtr elIt = GetElements();
      if ( elIt->more() ) {
        // find out nb of elements to skip w/o check before the 1st OK element
        const SMDS_MeshElement* e = me->setNbElemToSkip( elIt );
        ++me->myMeshInfo[ e->GetEntityType() ];
        while ( elIt->more() )
          ++me->myMeshInfo[ elIt->next()->GetEntityType() ];
      }
    }
    me->setChanged( false );
  }
}

//================================================================================
/*!
 * \brief Updates myElements in parallel
 */
//================================================================================
#ifdef WITH_TBB

#ifdef WIN32
// See https://docs.microsoft.com/en-gb/cpp/porting/modifying-winver-and-win32-winnt?view=vs-2019
// Windows 10 = 0x0A00  
#define WINVER 0x0A00
#define _WIN32_WINNT 0x0A00

#endif

#include <tbb/parallel_for.h>
#include "tbb/enumerable_thread_specific.h"

// a predicate per a thread
typedef tbb::enumerable_thread_specific<SMESH_PredicatePtr> TLocalPredicat;

struct IsSatisfyParallel
{
  vector< char >&     myIsElemOK;
  SMESH_PredicatePtr  myPredicate;
  TLocalPredicat&     myLocalPredicates;
  IsSatisfyParallel( SMESH_PredicatePtr mainPred, TLocalPredicat& locPred, vector< char >& isOk )
    : myIsElemOK(isOk), myPredicate( mainPred ), myLocalPredicates( locPred )
  {}
  void operator() ( const tbb::blocked_range<size_t>& r ) const
  {
    SMESH_PredicatePtr& pred = myLocalPredicates.local();
    if ( !pred )
    {
      if ( r.begin() == 0 )
        pred = myPredicate;
      else
        pred.reset( myPredicate->clone() );
    }
    for ( size_t i = r.begin(); i != r.end(); ++i )
      myIsElemOK[ i ] = char( pred->IsSatisfy( i ));
  }
};

bool SMESHDS_GroupOnFilter::updateParallel() const
{
  // if ( !getenv("updateParallel"))
  //   return false;
  size_t nbElemsOfType = GetMesh()->GetMeshInfo().NbElements( GetType() );
  if ( nbElemsOfType == 0 )
    return true;
  if ( nbElemsOfType < 1000000 )
    return false; // no sense in parallel work

  SMDS_ElemIteratorPtr elemIt = GetMesh()->elementsIterator( GetType() );
  const smIdType minID = elemIt->next()->GetID();
  myPredicate->IsSatisfy( minID ); // make myPredicate fully initialized for clone()
  SMESH_PredicatePtr clone( myPredicate->clone() );
  if ( !clone )
    return false;

  TLocalPredicat threadPredicates;
  threadPredicates.local() = clone;

  smIdType maxID = ( GetType() == SMDSAbs_Node ) ? GetMesh()->MaxNodeID() : GetMesh()->MaxElementID();
  vector< char > isElemOK( 1 + maxID );

  tbb::parallel_for ( tbb::blocked_range<size_t>( 0, isElemOK.size() ),
                      IsSatisfyParallel( myPredicate, threadPredicates, isElemOK ),
                      tbb::simple_partitioner());

  SMESHDS_GroupOnFilter* me = const_cast<SMESHDS_GroupOnFilter*>( this );

  int nbOkElems = 0;
  for ( size_t i = minID; i < isElemOK.size(); ++i )
    nbOkElems += ( isElemOK[ i ]);
  me->myElements.resize( nbOkElems );

  const SMDS_MeshElement* e;
  size_t iElem = 0;
  if ( GetType() == SMDSAbs_Node )
  {
    for ( size_t i = minID; i < isElemOK.size(); ++i )
      if (( isElemOK[ i ] ) &&
          ( e = GetMesh()->FindNode( i )))
      {
        me->myElements[ iElem++ ] = e;
      }
    me->myMeshInfo[ SMDSEntity_Node ] = myElements.size();
  }
  else
  {
    for ( size_t i = minID; i < isElemOK.size(); ++i )
      if (( isElemOK[ i ] ) &&
          ( e = GetMesh()->FindElement( i )) &&
          ( e->GetType() == GetType() ))
      {
        me->myElements[ iElem++ ] = e;
        ++me->myMeshInfo[ e->GetEntityType() ];
      }
  }
  me->myElementsOK = ( iElem < nbElemsOfType );
  if ( !myElementsOK )
    clearVector( me->myElements ); // all elements satisfy myPredicate
  else
    me->myElements.resize( iElem );

  me->setChanged( false );
  return true;
}
#else

bool SMESHDS_GroupOnFilter::updateParallel() const
{
  return false;
}

#endif

//================================================================================
/*!
 * \brief Sets myMeshModifTime and clear fields according to modification state
 */
//================================================================================

void SMESHDS_GroupOnFilter::setChanged(bool changed)
{
  myMeshModifTime = GetMesh()->GetMTime();
  if ( changed && myMeshModifTime != 0 )
    --myMeshModifTime;
  if ( changed ) {
    clearVector( myElements );
    myElementsOK = false;
    myNbElemToSkip = 0;
    myMeshInfo.assign( SMDSEntity_Last, 0 );
  }
}

//================================================================================
/*!
 * \brief Sets myNbElemToSkip
 *  \param okElemIt - iterator on OK elements
 *  \retval const SMDS_MeshElement* - the first OK element
 */
//================================================================================

const SMDS_MeshElement*
SMESHDS_GroupOnFilter::setNbElemToSkip( SMDS_ElemIteratorPtr& okElemIt )
{
  // find out nb of elements to skip w/o check before the 1st OK element
  const SMDS_MeshElement* firstOkElem = okElemIt->next();
  if ( myNbElemToSkip == 0 )
  {
    SMDS_ElemIteratorPtr elemIt = GetMesh()->elementsIterator( GetType() );
    myNbElemToSkip = 0;
    while ( elemIt->next() != firstOkElem )
      ++myNbElemToSkip;
  }
  return firstOkElem;
}

//================================================================================
/*!
 * \brief Return elements before mesh compacting
 */
//================================================================================

SMDS_ElemIteratorPtr SMESHDS_GroupOnFilter::getElements()
{
  return boost::make_shared< SMDS_ElementVectorIterator >( myElements.begin(), myElements.end() );
}

//================================================================================
/*!
 * \brief clear myElements before re-filling after mesh compacting
 */
//================================================================================

void SMESHDS_GroupOnFilter::tmpClear()
{
  std::vector< const SMDS_MeshElement*> newElems( myElements.size() );
  myElements.swap( newElems );
  myElements.clear();
}

//================================================================================
/*!
 * \brief Re-fill myElements after mesh compacting
 */
//================================================================================

void SMESHDS_GroupOnFilter::add( const SMDS_MeshElement* element )
{
  myElements.push_back( element );
}
