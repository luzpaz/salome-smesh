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
  ${PROJECT_SOURCE_DIR}/src/SMESHDS
)

# additional preprocessor / compiler flags
ADD_DEFINITIONS(
  ${OpenCASCADE_DEFINITIONS}
  ${BOOST_DEFINITIONS}
  "-DBOOST_SYSTEM_NO_DEPRECATED"
)

SET(_link_LIBRARIES
  ${Boost_LIBRARIES}
  ${SALOMEBOOTSTRAP_SALOMEBasics}
  MeshDriver
  SMESHUtils
)

# --- headers ---

# header files / no moc processing
SET(MeshDriverGMF_HEADERS
  DriverGMF_Read.hxx
  DriverGMF_Write.hxx
  DriverGMF.hxx
  SMESH_DriverGMF.hxx
  libmesh5.h
)

# --- sources ---

# sources / static
SET(MeshDriverGMF_SOURCES
  DriverGMF_Read.cxx
  DriverGMF_Write.cxx
  DriverGMF.cxx
  libmesh5.c
)

# --- rules ---

ADD_LIBRARY(MeshDriverGMF ${MeshDriverGMF_SOURCES})
TARGET_LINK_LIBRARIES(MeshDriverGMF ${_link_LIBRARIES} )
INSTALL(TARGETS MeshDriverGMF EXPORT ${PROJECT_NAME}TargetGroup DESTINATION ${SALOME_INSTALL_LIBS})

INSTALL(FILES ${MeshDriverGMF_HEADERS} DESTINATION ${SALOME_INSTALL_HEADERS})
