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

//  SMESH SMDS : implementation of Salome mesh data structure
//  File   : SMDS_FacePosition.hxx
//  Module : SMESH
//
#ifndef _SMDS_FacePosition_HeaderFile
#define _SMDS_FacePosition_HeaderFile

#include "SMESH_SMDS.hxx"

#include "SMDS_Position.hxx"

class SMDS_EXPORT SMDS_FacePosition:public SMDS_Position
{
 public:
  SMDS_FacePosition(double aUParam=0, double aVParam=0);
  SMDS_TypeOfPosition GetTypeOfPosition() const;
  virtual void SetUParameter(double aUparam);
  virtual void SetVParameter(double aVparam);
  virtual void SetParameters(double aUparam, double aVparam);
  virtual double GetUParameter() const;
  virtual double GetVParameter() const;
  virtual const double* GetParameters() const { return &myParameter[0]; }

 private:
  double myParameter[2];
};
#endif
