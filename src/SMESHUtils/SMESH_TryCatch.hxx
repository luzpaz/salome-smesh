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
// File      : SMESH_TryCatch.hxx
// Created   : Mon Dec 17 15:43:38 2012
// Author    : Edward AGAPOV (eap)

#ifndef __SMESH_TryCatch_HXX__
#define __SMESH_TryCatch_HXX__

#include "SMESH_Comment.hxx"
#include "SMESH_ComputeError.hxx"
#include "SMESH_Utils.hxx"

#include <Utils_SALOME_Exception.hxx>
#include <Standard_Failure.hxx>
#include <Standard_ErrorHandler.hxx>
#include <utilities.h>

// IMPORTANT: include this file _after_ OCC ones, else OCC_CATCH_SIGNALS can be undefined!

#ifndef OCC_CATCH_SIGNALS
#define OCC_CATCH_SIGNALS
#endif

// Define macros to catch and convert some of possible exceptions into text or SALOME_Exception.
// WARNING: SALOME::SALOME_Exception (CORBA exception) is not treated here; to care about it add
//          #define SMY_OWN_CATCH catch ( SALOME::SALOME_Exception & e ) { do_something(e); }
//          before #include<SMESH_TryCatch.hxx>


//-------------------------------------------------------------------------------------
#define SMESH_TRY                               \
  try {                                         \
  OCC_CATCH_SIGNALS                             \

//-------------------------------------------------------------------------------------
// A macro to add a custom catch clause to SMESH_CATCH
// To add your own catch close, define SMY_OWN_CATCH macro before including this file.
#ifndef SMY_OWN_CATCH
#define SMY_OWN_CATCH
#endif

//-------------------------------------------------------------------------------------
// A macro allowing to retrieve a result returned by onExceptionFun
#define SMESH_CAUGHT

//-------------------------------------------------------------------------------------
// A macro makes description of a caught exception and calls onExceptionFun(const char*).
// Several onExceptionFun() are defined here: throwSalomeEx(), doNothing() and returnError().
// To add your own catch clause, define SMY_OWN_CATCH macro before including this file.

#define SMESH_CATCH( onExceptionFun )                                   \
  }                                                                     \
                                                                        \
  SMY_OWN_CATCH                                                         \
                                                                        \
  catch (Standard_Failure& ex)                                          \
  {                                                                     \
    SMESH_Comment text("OCCT Exception: ");                             \
    text << ": " << ex.DynamicType()->Name();                           \
    if ( ex.GetMessageString() && strlen( ex.GetMessageString() ))      \
      text << ": " << ex.GetMessageString();                            \
    SMESH_CAUGHT onExceptionFun( text );                                \
  }                                                                     \
  catch ( ::SMESH_ComputeError& ce )                                    \
  {                                                                     \
    if ( !ce.myComment.empty() )                                        \
      SMESH_CAUGHT onExceptionFun( ce.myComment.c_str() );              \
    else if ( ce.IsCommon() )                                           \
      SMESH_CAUGHT onExceptionFun( ce.CommonName().c_str() );           \
    else                                                                \
      SMESH_CAUGHT onExceptionFun                                       \
        (SMESH_Comment("SMESH_ComputeError: ") << ce.myName );          \
  }                                                                     \
  catch ( const std::exception& ex)                                     \
  {                                                                     \
    SMESH_CAUGHT onExceptionFun( ex.what() );                           \
  }                                                                     \
  catch (...)                                                           \
  {                                                                     \
    SMESH_CAUGHT onExceptionFun("Unknown Exception caught");            \
  }

//-------------------------------------------------------------------------------------
// Functions that can be used as an argument of SMESH_CATCH

namespace SMESH
{
  SMESHUtils_EXPORT void throwSalomeEx(const char* txt);
  SMESHUtils_EXPORT void doNothing(const char* txt);
  SMESHUtils_EXPORT const char* returnError(const char* txt);
  SMESHUtils_EXPORT void printErrorInDebugMode(const char* txt);
}

#endif
