# Copyright (C) 2012-2025  CEA, EDF, OPEN CASCADE
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

# --- options ---
# additional include directories
INCLUDE_DIRECTORIES(
  ${SALOMEBOOTSTRAP_INCLUDE_DIRS}
  ${KERNEL_INCLUDE_DIRS}
  ${OpenCASCADE_INCLUDE_DIR}
  ${Boost_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/Driver
  ${PROJECT_SOURCE_DIR}/src/SMDS
  ${PROJECT_SOURCE_DIR}/src/SMESHUtils
  ${PROJECT_SOURCE_DIR}/src/SMESH
)

# additional preprocessor / compiler flags
ADD_DEFINITIONS(
  ${OpenCASCADE_DEFINITIONS}
  ${BOOST_DEFINITIONS}
)

# libraries to link to
SET(_link_LIBRARIES
  ${OpenCASCADE_FoundationClasses_LIBRARIES}
  ${OpenCASCADE_DataExchange_LIBRARIES}
  ${OpenCASCADE_ModelingAlgorithms_LIBRARIES}
  ${SALOMEBOOTSTRAP_SALOMEBasics}
  MeshDriver
  SMDS
  SMESHUtils
)

SET(_link_LIBRARIES_bin
  ${KERNEL_OpUtil}
  ${SALOMEBOOTSTRAP_SALOMELocalTrace}
  ${SALOMEBOOTSTRAP_SALOMEBasics}
  ${KERNEL_SalomeIDLKERNEL}
  ${OpenCASCADE_FoundationClasses_LIBRARIES}
  ${OpenCASCADE_DataExchange_LIBRARIES}
  ${OpenCASCADE_ModelingAlgorithms_LIBRARIES}
  MeshDriver
  SMDS
)

# --- headers ---

# header files / no moc processing
SET(MeshDriverSTL_HEADERS
  DriverSTL_R_SMDS_Mesh.h
  DriverSTL_W_SMDS_Mesh.h
  SMESH_DriverSTL.hxx
)

# --- sources ---

# sources / static
SET(MeshDriverSTL_SOURCES
  DriverSTL_R_SMDS_Mesh.cxx
  DriverSTL_W_SMDS_Mesh.cxx
)

# --- rules ---

ADD_LIBRARY(MeshDriverSTL ${MeshDriverSTL_SOURCES})
TARGET_LINK_LIBRARIES(MeshDriverSTL ${_link_LIBRARIES} )
INSTALL(TARGETS MeshDriverSTL EXPORT ${PROJECT_NAME}TargetGroup DESTINATION ${SALOME_INSTALL_LIBS})

INSTALL(FILES ${MeshDriverSTL_HEADERS} DESTINATION ${SALOME_INSTALL_HEADERS})
