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
  ${MEDFILE_INCLUDE_DIRS}
  ${GEOM_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${OpenCASCADE_INCLUDE_DIR}
  ${OMNIORB_INCLUDE_DIR}
  ${MEDCOUPLING_INCLUDE_DIRS}
  ${PROJECT_SOURCE_DIR}/src/Controls
  ${PROJECT_SOURCE_DIR}/src/Driver
  ${PROJECT_SOURCE_DIR}/src/DriverDAT
  ${PROJECT_SOURCE_DIR}/src/DriverMED
  ${PROJECT_SOURCE_DIR}/src/DriverUNV
  ${PROJECT_SOURCE_DIR}/src/DriverSTL
  ${PROJECT_SOURCE_DIR}/src/SMDS
  ${PROJECT_SOURCE_DIR}/src/SMESHDS
  ${PROJECT_SOURCE_DIR}/src/SMESH
  ${PROJECT_SOURCE_DIR}/src/SMESHUtils
  ${PROJECT_SOURCE_DIR}/src/SMESH_I
  ${PROJECT_BINARY_DIR}/idl
)

# additional preprocessor / compiler flags
ADD_DEFINITIONS(
  ${OMNIORB_DEFINITIONS}
  ${OpenCASCADE_DEFINITIONS}
  ${BOOST_DEFINITIONS}
)

# libraries to link to
SET(_link_LIBRARIES
  ${KERNEL_SalomeLifeCycleCORBA}
  ${KERNEL_SalomeNS}
  ${KERNEL_OpUtil}
  ${SALOMEBOOTSTRAP_SALOMEBasics}
  ${KERNEL_SalomeContainer}
  ${KERNEL_SalomeHDFPersist}
  ${KERNEL_SalomeGenericObj}
  ${KERNEL_Registry}
  ${KERNEL_ResourcesManager}
  ${KERNEL_SalomeResourcesManager}
  ${KERNEL_SalomeNotification}
  ${KERNEL_SalomeIDLKERNEL}
  ${SALOMEBOOTSTRAP_SALOMELocalTrace}
  ${GEOM_NMTDS}
  ${GEOM_SalomeIDLGEOM}  
  ${OpenCASCADE_KERNEL}
  SalomeIDLSMESH
  SMDS
  SMESHimpl
  SMESHDS
  SMESHControls
  SMESHEngine 
  MEDWrapper
)

# --- headers ---

# header files / no moc processing
SET(SMESHClient_HEADERS
  SMESH_Client.hxx
)

# --- sources ---

# sources / static
SET(SMESHClient_SOURCES
  SMESH_Client.cxx
)

# --- rules ---

ADD_LIBRARY(SMESHClient ${SMESHClient_SOURCES})
TARGET_LINK_LIBRARIES(SMESHClient ${_link_LIBRARIES} )
INSTALL(TARGETS SMESHClient EXPORT ${PROJECT_NAME}TargetGroup DESTINATION ${SALOME_INSTALL_LIBS})


INSTALL(FILES ${SMESHClient_HEADERS} DESTINATION ${SALOME_INSTALL_HEADERS})
