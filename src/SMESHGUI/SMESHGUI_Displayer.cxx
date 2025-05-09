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

// SMESH SMESHGUI : Displayer for SMESH module
// File   : SMESHGUI_Displayer.cxx
// Author : Alexander SOLOVYOV, Open CASCADE S.A.S.
// SMESH includes
//

#include "SMESHGUI_Displayer.h"
#include "SMESHGUI_VTKUtils.h"
#include "SMESHGUI_Utils.h"

// SALOME GUI includes
#include <SalomeApp_Study.h>
#include <SalomeApp_Application.h>
#include <SUIT_ViewManager.h>
#include <SVTK_ViewModel.h>
#include <SVTK_ViewWindow.h>
#include <SPV3D_Prs.h>
#include <SPV3D_ViewModel.h>
#include <PV3DViewer_ViewWindow.h>

//For PV3D
#include "SMESH_Actor.h"

// IDL includes
#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SMESH_Group)
#include CORBA_SERVER_HEADER(SMESH_Mesh)

std::string SMESHGUI_Displayer::getName( const QString& entry )
{
  Handle( SALOME_InteractiveObject ) theIO = new SALOME_InteractiveObject();
  theIO->setEntry( entry.toUtf8().constData() );
  if ( !theIO.IsNull() )
  {
    //  Find SOBject (because shape should be published previously)
    if ( study() )
    {
      _PTR(SObject) aSObj ( study()->studyDS()->FindObjectID( theIO->getEntry() ) );
      _PTR(GenericAttribute) anAttr;

      if ( aSObj && aSObj->FindAttribute( anAttr, "AttributeName") )
      {
        _PTR(AttributeName) aNameAttr( anAttr );
        return aNameAttr->Value();
      }
    }
  }
  return "";
}

SMESHGUI_Displayer::SMESHGUI_Displayer( SalomeApp_Application* app )
: LightApp_Displayer(),
  myApp( app ),
  isNeedFitAll(false)
{
}

SMESHGUI_Displayer::~SMESHGUI_Displayer()
{
}

SALOME_Prs* SMESHGUI_Displayer::buildPresentation( const QString& entry, SALOME_View* theViewFrame )
{
  SALOME_Prs *prs = nullptr;

  SALOME_View* aViewFrame = theViewFrame ? theViewFrame : GetActiveView();

  if ( aViewFrame )
  {
    SVTK_Viewer* vtk_viewer = dynamic_cast<SVTK_Viewer*>( aViewFrame );
    if( vtk_viewer )
    {
      SUIT_ViewWindow* wnd = vtk_viewer->getViewManager()->getActiveView();
      SMESH_Actor* anActor = SMESH::FindActorByEntry( wnd, entry.toUtf8().data() );
      if( !anActor )
        anActor = SMESH::CreateActor( entry.toUtf8().data(), true );
      if( anActor )
      {
        isNeedFitAll = SMESH::NoSmeshActors();
        SMESH::DisplayActor( wnd, anActor );
        prs = LightApp_Displayer::buildPresentation( entry.toUtf8().data(), aViewFrame );
      }
      if( prs )
        UpdatePrs( prs );
      else if( anActor )
        SMESH::RemoveActor( vtk_viewer->getViewManager()->getActiveView(), anActor );
    }
    
    SPV3D_ViewModel *pv3d_viewer = dynamic_cast<SPV3D_ViewModel *>( aViewFrame );
    if(pv3d_viewer)
    {
      SUIT_ViewWindow* wnd = pv3d_viewer->getViewManager()->getActiveView();
      SMESH_Actor* anActor = SMESH::FindActorByEntry( wnd, entry.toUtf8().data() );
      if( !anActor )
        anActor = SMESH::CreateActor( entry.toUtf8().data(), true );
      if( anActor )
      {
        prs = LightApp_Displayer::buildPresentation( entry.toUtf8().data(), aViewFrame );
        if( prs )
        {
          SPV3D_Prs *pv3dPrs = dynamic_cast<SPV3D_Prs*>( prs );
          if( pv3dPrs )
          {
            pv3dPrs->SetName( getName( entry ) );
            pv3dPrs->FillUsingActor( anActor );
          }
        }
      }
    }
  }

  return prs;
}

SalomeApp_Study* SMESHGUI_Displayer::study() const
{
  return dynamic_cast<SalomeApp_Study*>( myApp->activeStudy() );
}

bool SMESHGUI_Displayer::canBeDisplayed( const QString& entry, const QString& viewer_type ) const
{
  bool res = false;
  if(viewer_type != SVTK_Viewer::Type() && viewer_type != SPV3D_ViewModel::Type())
    return res;
  
  _PTR(SObject) obj = SMESH::getStudy()->FindObjectID( (const char*)entry.toUtf8() );
  CORBA::Object_var anObj = SMESH::SObjectToObject( obj );
  
  if ( !CORBA::is_nil( anObj ) )
  {
    SMESH::SMESH_Mesh_var mesh = SMESH::SMESH_Mesh::_narrow( anObj );
    if ( ! ( res = !mesh->_is_nil() ))
    {
      SMESH::SMESH_subMesh_var aSubMeshObj = SMESH::SMESH_subMesh::_narrow( anObj );
      if ( ! ( res = !aSubMeshObj->_is_nil() ))
      {
        SMESH::SMESH_GroupBase_var aGroupObj = SMESH::SMESH_GroupBase::_narrow( anObj );
        res = !aGroupObj->_is_nil();
      }
    }
  }
  return res;
}

void SMESHGUI_Displayer::Display( const QStringList& theList, const bool anUpdateViewer, SALOME_View* theView ) 
{
  LightApp_Displayer::Display( theList, anUpdateViewer, theView );
  
  if (isNeedFitAll) {
    SMESH::FitAll();
    isNeedFitAll = false;
  }
}