#  -*- coding: iso-8859-1 -*-
# Copyright (C) 2007-2015  CEA/DEN, EDF R&D, OPEN CASCADE
#
# Copyright (C) 2003-2007  OPEN CASCADE, EADS/CCR, LIP6, CEA/DEN,
# CEDRAT, EDF R&D, LEG, PRINCIPIA R&D, BUREAU VERITAS
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
#
# See http://www.salome-platform.org/ or email : webmaster.salome@opencascade.com
#

# Tetrahedrization of the geometry generated by the Python script
# SMESH_fixation.py
# The new Netgen algorithm is used that discretizes baoundaries itself
#
import salome
import SMESH_fixation

import SMESH, SALOMEDS
from salome.smesh import smeshBuilder
smesh =  smeshBuilder.New(salome.myStudy)

compshell = SMESH_fixation.compshell
idcomp = SMESH_fixation.idcomp
geompy = SMESH_fixation.geompy
salome = SMESH_fixation.salome

print "Analysis of the geometry to be meshed :"
subShellList = geompy.SubShapeAll(compshell, geompy.ShapeType["SHELL"])
subFaceList  = geompy.SubShapeAll(compshell, geompy.ShapeType["FACE"])
subEdgeList  = geompy.SubShapeAll(compshell, geompy.ShapeType["EDGE"])

print "number of Shells in compshell : ", len(subShellList)
print "number of Faces  in compshell : ", len(subFaceList)
print "number of Edges  in compshell : ", len(subEdgeList)

status = geompy.CheckShape(compshell)
print " check status ", status

### ---------------------------- SMESH --------------------------------------
smesh.SetCurrentStudy(salome.myStudy)

print "-------------------------- create Mesh, algorithm, hypothesis"

mesh = smesh.Mesh(compshell, "MeshcompShel");
netgen = mesh.Tetrahedron(smeshBuilder.FULL_NETGEN)
netgen.SetMaxSize( 50 )
#netgen.SetSecondOrder( 0 )
netgen.SetFineness( smeshBuilder.Fine )
#netgen.SetOptimize( 1 )

print "-------------------------- compute mesh"
ret = mesh.Compute()
print ret
if ret != 0:
    print "Information about the MeshcompShel:"
    print "Number of nodes        : ", mesh.GetMesh().NbNodes()
    print "Number of edges        : ", mesh.GetMesh().NbEdges()
    print "Number of faces        : ", mesh.GetMesh().NbFaces()
    print "Number of triangles    : ", mesh.GetMesh().NbTriangles()
    print "Number of volumes      : ", mesh.GetMesh().NbVolumes()
    print "Number of tetrahedrons : ", mesh.GetMesh().NbTetras()
    
else:
    print "problem when computing the mesh"

salome.sg.updateObjBrowser(1)
