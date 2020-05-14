// Copyright (C) 2007-2020  CEA/DEN, EDF R&D, OPEN CASCADE
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
//  File   : SMESH_Gen_i.hxx
//  Author : Paul RASCLE, EDF
//  Module : SMESH

#ifndef _SMESH_GEN_I_HXX_
#define _SMESH_GEN_I_HXX_

#include "SMESH.hxx"

#include <SALOMEconfig.h>
#include CORBA_SERVER_HEADER(SMESH_Gen)
#include CORBA_SERVER_HEADER(SMESH_Mesh)
#include CORBA_SERVER_HEADER(SMESH_Hypothesis)
#include CORBA_CLIENT_HEADER(GEOM_Gen)
#include CORBA_CLIENT_HEADER(SALOMEDS)
#include CORBA_CLIENT_HEADER(SALOMEDS_Attributes)

#include "SMESH_Gen.hxx"
#include "SMESH_Mesh_i.hxx"
#include "SMESH_Hypothesis_i.hxx"

#include <SALOME_Component_i.hxx>
#include <SALOME_NamingService.hxx>
#include <Utils_CorbaException.hxx>

#include <GEOM_Client.hxx>

#include <TCollection_AsciiString.hxx>
#include <Resource_DataMapOfAsciiStringAsciiString.hxx>
#include <TColStd_HSequenceOfAsciiString.hxx>
#include <NCollection_DataMap.hxx>

#include <map>
#include <sstream>

class SMESH_Mesh_i;
class SALOME_LifeCycleCORBA;

// ===========================================================
// Study context - store study-connected objects references
// ==========================================================
class SMESH_I_EXPORT StudyContext
{
  typedef NCollection_DataMap< int, std::string > TInt2StringMap;
  typedef NCollection_DataMap< int, int >         TInt2IntMap;
public:
  // constructor
  StudyContext() {}
  // register object in the internal map and return its id
  int         addObject( const std::string& theIOR );
  // find the object id in the internal map by the IOR
  int         findId( const std::string& theIOR );
  // get object's IOR by id
  std::string getIORbyId( const int theId );
  // get object's IOR by old id
  std::string getIORbyOldId( const int theOldId );
  // maps old object id to the new one (used when restoring data)
  void        mapOldToNew( const int oldId, const int newId );
  // get old id by a new one
  int         getOldId( const int newId );
  // clear data
  void        Clear();

private:
  // get next free object identifier
  int         getNextId() { return mapIdToIOR.Extent() + 1; }

  TInt2StringMap mapIdToIOR; // persistent-to-transient map
  TInt2IntMap    mapIdToId;  // to translate object from persistent to transient form
};

// ===========================================================
// SMESH module's engine
// ==========================================================
class SMESH_I_EXPORT SMESH_Gen_i:
  public virtual POA_SMESH::SMESH_Gen,
  public virtual Engines_Component_i
{
public:
  // Get last created instance of the class
  static SMESH_Gen_i* GetSMESHGen() { return mySMESHGen;}
  // Get ORB object
  static CORBA::ORB_var GetORB() { return myOrb;}
  // Get SMESH module's POA object
  static PortableServer::POA_var GetPOA() { return myPoa;}
  // Get Naming Service object
  static SALOME_NamingService* GetNS();
  // Get SALOME_LifeCycleCORBA object
  static SALOME_LifeCycleCORBA* GetLCC();
  // Retrieve and get GEOM engine reference
  static GEOM::GEOM_Gen_var GetGeomEngine( bool isShaper );
  static GEOM::GEOM_Gen_var GetGeomEngine( GEOM::GEOM_Object_ptr );
  // Get object of the CORBA reference
  static PortableServer::ServantBase_var GetServant( CORBA::Object_ptr theObject );
  // Get CORBA object corresponding to the SALOMEDS::SObject
  static CORBA::Object_var SObjectToObject( SALOMEDS::SObject_ptr theSObject );
  // Get the SALOMEDS::SObject corresponding to a CORBA object
  static SALOMEDS::SObject_ptr ObjectToSObject(CORBA::Object_ptr theObject);
  // Get the SALOMEDS::Study from naming service
  static SALOMEDS::Study_var getStudyServant();
  // Get GEOM Object corresponding to TopoDS_Shape
  GEOM::GEOM_Object_ptr ShapeToGeomObject (const TopoDS_Shape& theShape );
  // Get TopoDS_Shape corresponding to GEOM_Object
  TopoDS_Shape GeomObjectToShape(GEOM::GEOM_Object_ptr theGeomObject);

  // Default constructor
  SMESH_Gen_i();
  // Standard constructor
  SMESH_Gen_i( CORBA::ORB_ptr            orb,
               PortableServer::POA_ptr   poa,
               PortableServer::ObjectId* contId,
               const char*               instanceName,
               const char*               interfaceName );
  // Destructor
  virtual ~SMESH_Gen_i();

  // *****************************************
  // Interface methods
  // *****************************************
  // Set a new Mesh object name
  void SetName(const char* theIOR,
               const char* theName);

  //GEOM::GEOM_Gen_ptr SetGeomEngine( const char* containerLoc );
  void SetGeomEngine( GEOM::GEOM_Gen_ptr geomcompo );

  // Set embedded mode
  void SetEmbeddedMode( CORBA::Boolean theMode );
  // Check embedded mode
  CORBA::Boolean IsEmbeddedMode();

  // Set enable publishing in the study
  void SetEnablePublish( CORBA::Boolean theIsEnablePublish );

  // Check enable publishing
  CORBA::Boolean IsEnablePublish();

  // Update study
  void UpdateStudy();

  // Do provide info on objects
  bool hasObjectInfo();

  // Return an information for a given object
  char* getObjectInfo(const char* entry);

  // Create hypothesis/algorithm of given type
  SMESH::SMESH_Hypothesis_ptr CreateHypothesis (const char* theHypType,
                                                const char* theLibName)
    throw ( SALOME::SALOME_Exception );

  SMESH::SMESH_Hypothesis_ptr CreateHypothesisByAverageLength( const char*    theHypType,
                                                               const char*    theLibName,
                                                               CORBA::Double  theAverageLength,
                                                               CORBA::Boolean theQuadDominated)
    throw ( SALOME::SALOME_Exception );

  // Return hypothesis of given type holding parameter values of the existing mesh
  SMESH::SMESH_Hypothesis_ptr
    GetHypothesisParameterValues (const char*                 theHypType,
                                  const char*                 theLibName,
                                  SMESH::SMESH_Mesh_ptr       theMesh,
                                  GEOM::GEOM_Object_ptr       theGeom,
                                  const SMESH::HypInitParams& theWay)
    throw ( SALOME::SALOME_Exception );

  /*
   * Returns True if a hypothesis is assigned to a sole sub-mesh in a current Study
   */
  CORBA::Boolean GetSoleSubMeshUsingHyp( SMESH::SMESH_Hypothesis_ptr theHyp,
                                         SMESH::SMESH_Mesh_out       theMesh,
                                         GEOM::GEOM_Object_out       theShape);

  // Preferences
  // ------------
  /*!
   * Sets number of segments per diagonal of boundary box of geometry by which
   * default segment length of appropriate 1D hypotheses is defined
   */
  void SetBoundaryBoxSegmentation( CORBA::Long theNbSegments ) throw ( SALOME::SALOME_Exception );
  /*!
   * \brief Sets default number of segments per edge
   */
  void SetDefaultNbSegments(CORBA::Long theNbSegments) throw ( SALOME::SALOME_Exception );

  /*!
    Set an option value
  */
  virtual void  SetOption(const char*, const char*);
  /*!
    Return an option value
  */
  virtual char* GetOption(const char*);

  /*!
   * To load full mesh data from study at hyp modification or not
   */
  bool ToForgetMeshDataOnHypModif() const { return myToForgetMeshDataOnHypModif; }


  // Create empty mesh on a shape
  SMESH::SMESH_Mesh_ptr CreateMesh( GEOM::GEOM_Object_ptr theShapeObject )
    throw ( SALOME::SALOME_Exception );

  // Create empty mesh
  SMESH::SMESH_Mesh_ptr CreateEmptyMesh()
    throw ( SALOME::SALOME_Exception );

  //  Create a mesh and import data from an UNV file
  SMESH::SMESH_Mesh_ptr CreateMeshesFromUNV( const char* theFileName )
    throw ( SALOME::SALOME_Exception );

  //  Create mesh(es) and import data from MED file
  SMESH::mesh_array* CreateMeshesFromMED( const char* theFileName,
                                          SMESH::DriverMED_ReadStatus& theStatus )
    throw ( SALOME::SALOME_Exception );

  //  Create mesh(es) and import data from MED file
  SMESH::mesh_array* CreateMeshesFromSAUV( const char* theFileName,
                                           SMESH::DriverMED_ReadStatus& theStatus )
    throw ( SALOME::SALOME_Exception );

  //  Create a mesh and import data from a STL file
  SMESH::SMESH_Mesh_ptr CreateMeshesFromSTL( const char* theFileName )
    throw ( SALOME::SALOME_Exception );

  //  Create mesh(es) and import data from CGNS file
  SMESH::mesh_array* CreateMeshesFromCGNS( const char* theFileName,
                                           SMESH::DriverMED_ReadStatus& theStatus )
    throw ( SALOME::SALOME_Exception );

  //  Create a mesh and import data from a GMF file
  SMESH::SMESH_Mesh_ptr CreateMeshesFromGMF( const char*             theFileName,
                                             CORBA::Boolean          theMakeRequiredGroups,
                                             SMESH::ComputeError_out theError)
    throw ( SALOME::SALOME_Exception );

  // Copy a part of mesh
  SMESH::SMESH_Mesh_ptr CopyMesh(SMESH::SMESH_IDSource_ptr meshPart,
                                 const char*               meshName,
                                 CORBA::Boolean            toCopyGroups,
                                 CORBA::Boolean            toKeepIDs)
    throw ( SALOME::SALOME_Exception );

  // Create a mesh by copying definitions of another mesh to a given geometry
  CORBA::Boolean CopyMeshWithGeom( SMESH::SMESH_Mesh_ptr       sourceMesh,
                                   GEOM::GEOM_Object_ptr       newGeometry,
                                   const char*                 meshName,
                                   CORBA::Boolean              toCopyGroups,
                                   CORBA::Boolean              toReuseHypotheses,
                                   CORBA::Boolean              toCopyElements,
                                   SMESH::SMESH_Mesh_out       newMesh,
                                   SMESH::ListOfGroups_out     newGroups,
                                   SMESH::submesh_array_out    newSubmeshes,
                                   SMESH::ListOfHypothesis_out newHypotheses,
                                   SMESH::string_array_out     invalidEntries)
    throw ( SALOME::SALOME_Exception );

    // Compute mesh on a shape
  CORBA::Boolean Compute( SMESH::SMESH_Mesh_ptr theMesh,
                          GEOM::GEOM_Object_ptr theShapeObject )
    throw ( SALOME::SALOME_Exception );

  // Cancel Compute mesh on a shape
  void CancelCompute( SMESH::SMESH_Mesh_ptr theMesh,
                      GEOM::GEOM_Object_ptr theShapeObject );

  /*!
   * \brief Return errors of mesh computation
   */
  SMESH::compute_error_array* GetComputeErrors(SMESH::SMESH_Mesh_ptr theMesh,
                                               GEOM::GEOM_Object_ptr  theShapeObject )
    throw ( SALOME::SALOME_Exception );

  /*!
   * Evaluate mesh on a shape and
   *  returns statistic of mesh elements
   * Result array of number enityties
   */
  SMESH::long_array* Evaluate(SMESH::SMESH_Mesh_ptr theMesh,
                              GEOM::GEOM_Object_ptr theShapeObject)
    throw ( SALOME::SALOME_Exception );

  // Returns true if mesh contains enough data to be computed
  CORBA::Boolean IsReadyToCompute( SMESH::SMESH_Mesh_ptr theMesh,
                                   GEOM::GEOM_Object_ptr theShapeObject )
    throw ( SALOME::SALOME_Exception );

  /*!
   * Calculate Mesh as preview till indicated dimension on shape
   * First, verify list of hypothesis associated with the subShape.
   * Return mesh preview structure
   */
  SMESH::MeshPreviewStruct* Precompute( SMESH::SMESH_Mesh_ptr theMesh,
                                        GEOM::GEOM_Object_ptr theSubObject,
                                        SMESH::Dimension      theDimension,
                                        SMESH::long_array&    theShapesId )
    throw ( SALOME::SALOME_Exception );

  // Returns errors of hypotheses definition
  SMESH::algo_error_array* GetAlgoState( SMESH::SMESH_Mesh_ptr theMesh,
                                         GEOM::GEOM_Object_ptr theSubObject )
    throw ( SALOME::SALOME_Exception );

  // Return mesh elements preventing computation of a subshape
  SMESH::MeshPreviewStruct* GetBadInputElements( SMESH::SMESH_Mesh_ptr theMesh,
                                                 CORBA::Short          theSubShapeID )
    throw ( SALOME::SALOME_Exception );

  // Create groups of elements preventing computation of a sub-shape
  SMESH::ListOfGroups* MakeGroupsOfBadInputElements( SMESH::SMESH_Mesh_ptr theMesh,
                                                     CORBA::Short          theSubShapeID,
                                                     const char*           theGroupName)
    throw ( SALOME::SALOME_Exception );

  // Get sub-shapes unique ID's list
  SMESH::long_array* GetSubShapesId( GEOM::GEOM_Object_ptr      theMainShapeObject,
                                     const SMESH::object_array& theListOfSubShape )
    throw ( SALOME::SALOME_Exception );

  // Return geometrical object the given element is built on. Publish it in study.
  GEOM::GEOM_Object_ptr GetGeometryByMeshElement( SMESH::SMESH_Mesh_ptr  theMesh,
                                                  CORBA::Long            theElementID,
                                                  const char*            theGeomName)
    throw ( SALOME::SALOME_Exception );

  // Return geometrical object the given element is built on. Don't publish it in study.
  GEOM::GEOM_Object_ptr FindGeometryByMeshElement( SMESH::SMESH_Mesh_ptr  theMesh,
                                                   CORBA::Long            theElementID)
    throw ( SALOME::SALOME_Exception );

  // Concatenate the given meshes into one mesh
  SMESH::SMESH_Mesh_ptr ConcatenateCommon(const SMESH::ListOfIDSources& meshesArray,
                                          CORBA::Boolean                uniteIdenticalGroups,
                                          CORBA::Boolean                mergeNodesAndElements,
                                          CORBA::Double                 mergeTolerance,
                                          CORBA::Boolean                commonGroups,
                                          SMESH::SMESH_Mesh_ptr         meshToAppendTo)
    throw ( SALOME::SALOME_Exception );

  // Concatenate the given meshes into one mesh
  SMESH::SMESH_Mesh_ptr Concatenate(const SMESH::ListOfIDSources& meshesArray,
                                    CORBA::Boolean                uniteIdenticalGroups,
                                    CORBA::Boolean                mergeNodesAndElements,
                                    CORBA::Double                 mergeTolerance,
                                    SMESH::SMESH_Mesh_ptr         meshToAppendTo)
    throw ( SALOME::SALOME_Exception );

  // Concatenate the given meshes into one mesh
  // Create the groups of all elements from initial meshes
  SMESH::SMESH_Mesh_ptr ConcatenateWithGroups(const SMESH::ListOfIDSources& meshesArray,
                                              CORBA::Boolean                uniteIdenticalGroups,
                                              CORBA::Boolean                mergeNodesAndElements,
                                              CORBA::Double                 mergeTolerance,
                                              SMESH::SMESH_Mesh_ptr         meshToAppendTo)
    throw ( SALOME::SALOME_Exception );

  // Get version of MED format being used.
  char* GetMEDFileVersion();

  // Get MED version of the file by its name
  char* GetMEDVersion(const char* theFileName);

  // Check compatibility of file with MED format being used, read only.
  CORBA::Boolean CheckCompatibility(const char* theFileName);

  // Check compatibility of file with MED format being used, for append on write.
  CORBA::Boolean CheckWriteCompatibility(const char* theFileName);

  // Get names of meshes defined in file with the specified name
  SMESH::string_array* GetMeshNames(const char* theFileName);

  // ****************************************************
  // Interface inherited methods (from SALOMEDS::Driver)
  // ****************************************************

  // Save SMESH data
  SALOMEDS::TMPFile* Save( SALOMEDS::SComponent_ptr theComponent,
                         const char*              theURL,
                         bool                     isMultiFile );
  // Load SMESH data
  bool Load( SALOMEDS::SComponent_ptr theComponent,
             const SALOMEDS::TMPFile& theStream,
             const char*              theURL,
             bool                     isMultiFile );
  // Save SMESH data in ASCII format
  SALOMEDS::TMPFile* SaveASCII( SALOMEDS::SComponent_ptr theComponent,
                                const char*              theURL,
                                bool                     isMultiFile );
  // Load SMESH data in ASCII format
  bool LoadASCII( SALOMEDS::SComponent_ptr theComponent,
                  const SALOMEDS::TMPFile& theStream,
                  const char*              theURL,
                  bool                     isMultiFile );

  // Create filter manager
  SMESH::FilterManager_ptr CreateFilterManager();

  // Return a pattern mesher
  SMESH::SMESH_Pattern_ptr GetPattern();

  // Create measurement instance
  SMESH::Measurements_ptr  CreateMeasurements();

  // Clears study-connected data when it is closed
  void Close( SALOMEDS::SComponent_ptr theComponent );

  // Get component data type
  char* ComponentDataType();

  // Transform data from transient form to persistent
  char* IORToLocalPersistentID( SALOMEDS::SObject_ptr theSObject,
                                const char*           IORString,
                                CORBA::Boolean        isMultiFile,
                                CORBA::Boolean        isASCII );
  // Transform data from persistent form to transient
  char* LocalPersistentIDToIOR( SALOMEDS::SObject_ptr theSObject,
                                const char*           aLocalPersistentID,
                                CORBA::Boolean        isMultiFile,
                                CORBA::Boolean        isASCII );

  // Returns true if object can be published in the study
  bool CanPublishInStudy( CORBA::Object_ptr theIOR );
  // Publish object in the study
  SALOMEDS::SObject_ptr PublishInStudy( SALOMEDS::SObject_ptr theSObject,
                                        CORBA::Object_ptr     theObject,
                                        const char*           theName )
    throw ( SALOME::SALOME_Exception );

  // Copy-paste methods - returns true if object can be copied to the clipboard
  CORBA::Boolean CanCopy( SALOMEDS::SObject_ptr theObject ) { return false; }
  // Copy-paste methods - copy object to the clipboard
  SALOMEDS::TMPFile* CopyFrom( SALOMEDS::SObject_ptr theObject, CORBA::Long& theObjectID ) { return NULL; }
  // Copy-paste methods - returns true if object can be pasted from the clipboard
  CORBA::Boolean CanPaste( const char* theComponentName, CORBA::Long theObjectID ) { return false; }
  // Copy-paste methods - paste object from the clipboard
  SALOMEDS::SObject_ptr PasteInto( const SALOMEDS::TMPFile& theStream,
                                   CORBA::Long              theObjectID,
                                   SALOMEDS::SObject_ptr    theObject ) {
    SALOMEDS::SObject_var aResultSO;
    return aResultSO._retn();
  }

  // ============
  // Version information
  // ============

  virtual char* getVersion();

  // ============
  // Dump python
  // ============

  virtual Engines::TMPFile* DumpPython(CORBA::Boolean isPublished,
                                       CORBA::Boolean isMultiFile,
                                       CORBA::Boolean& isValidScript);

  void AddToPythonScript (const TCollection_AsciiString& theString);

  void RemoveLastFromPythonScript();

  void SavePython();

  TCollection_AsciiString DumpPython_impl (Resource_DataMapOfAsciiStringAsciiString& theObjectNames,
                                           Resource_DataMapOfAsciiStringAsciiString& theNames,
                                           bool isPublished,
                                           bool isMultiFile,
                                           bool isHistoricalDump,
                                           bool& aValidScript,
                                           TCollection_AsciiString& theSavedTrace);

  TCollection_AsciiString GetNewPythonLines();

  void CleanPythonTrace();

  // *****************************************
  // Internal methods
  // *****************************************
public:
  // Get shape reader
  GEOM_Client* GetShapeReader();

  // Tags definition
  static long GetHypothesisRootTag();
  static long GetAlgorithmsRootTag();
  static long GetRefOnShapeTag();
  static long GetRefOnAppliedHypothesisTag();
  static long GetRefOnAppliedAlgorithmsTag();
  static long GetSubMeshOnVertexTag();
  static long GetSubMeshOnEdgeTag();
  static long GetSubMeshOnFaceTag();
  static long GetSubMeshOnSolidTag();
  static long GetSubMeshOnCompoundTag();
  static long GetSubMeshOnWireTag();
  static long GetSubMeshOnShellTag();
  static long GetNodeGroupsTag();
  static long GetEdgeGroupsTag();
  static long GetFaceGroupsTag();
  static long GetVolumeGroupsTag();
  static long Get0DElementsGroupsTag();
  static long GetBallElementsGroupsTag();

  // publishing methods
  SALOMEDS::SComponent_ptr PublishComponent();
  SALOMEDS::SObject_ptr PublishMesh (SMESH::SMESH_Mesh_ptr theMesh,
                                     const char*           theName = 0);
  SALOMEDS::SObject_ptr PublishHypothesis (SMESH::SMESH_Hypothesis_ptr theHyp,
                                           const char*                 theName = 0);
  SALOMEDS::SObject_ptr PublishSubMesh (SMESH::SMESH_Mesh_ptr    theMesh,
                                        SMESH::SMESH_subMesh_ptr theSubMesh,
                                        GEOM::GEOM_Object_ptr    theShapeObject,
                                        const char*              theName = 0);
  SALOMEDS::SObject_ptr PublishGroup (SMESH::SMESH_Mesh_ptr  theMesh,
                                      SMESH::SMESH_GroupBase_ptr theGroup,
                                      GEOM::GEOM_Object_ptr  theShapeObject,
                                      const char*            theName = 0);
  void UpdateIcons(SMESH::SMESH_Mesh_ptr theMesh);
  void HighLightInvalid(CORBA::Object_ptr theObject, bool isInvalid);
  bool IsInvalid(SALOMEDS::SObject_ptr theObject);
  bool AddHypothesisToShape(SMESH::SMESH_Mesh_ptr       theMesh,
                            GEOM::GEOM_Object_ptr       theShapeObject,
                            SMESH::SMESH_Hypothesis_ptr theHyp);
  bool RemoveHypothesisFromShape(SMESH::SMESH_Mesh_ptr       theMesh,
                                 GEOM::GEOM_Object_ptr       theShapeObject,
                                 SMESH::SMESH_Hypothesis_ptr theHyp);
  SALOMEDS::SObject_ptr GetMeshOrSubmeshByShape (SMESH::SMESH_Mesh_ptr theMesh,
                                                 GEOM::GEOM_Object_ptr theShape);
  static void SetName(SALOMEDS::SObject_ptr theSObject,
                      const char*           theName,
                      const char*           theDefaultName = 0);

  static void SetPixMap(SALOMEDS::SObject_ptr theSObject,
                        const char*           thePixMap);

  //  Get study context
  StudyContext* GetStudyContext();

  // Register an object in a StudyContext; return object id
  int RegisterObject(CORBA::Object_ptr theObject);

  // Return id of registered object
  CORBA::Long GetObjectId(CORBA::Object_ptr theObject);

  // Return an object that previously had an oldID
  template<class TInterface>
    typename TInterface::_var_type GetObjectByOldId( const int oldID )
  {
    if ( myStudyContext ) {
      std::string ior = myStudyContext->getIORbyOldId( oldID );
      if ( !ior.empty() )
        return TInterface::_narrow(GetORB()->string_to_object( ior.c_str() ));
    }
    return TInterface::_nil();
  }

  /*!
   * \brief Find SObject for an algo
   */
  SALOMEDS::SObject_ptr GetAlgoSO(const ::SMESH_Algo* algo);

  void UpdateParameters(CORBA::Object_ptr theObject, const char* theParameters);
  char* GetParameters(CORBA::Object_ptr theObject);
  //char* ParseParameters(const char* theParameters);
  const std::vector< int >&         GetLastParamIndices() const { return myLastParamIndex; }
  const std::vector< std::string >& GetLastParameters() const { return myLastParameters; }
  const std::string &               GetLastObjEntry() const { return myLastObj; }
  std::vector< std::string >        GetAllParameters(const std::string& theObjectEntry) const;

  // Move objects to the specified position
  void Move( const SMESH::sobject_list& what,
             SALOMEDS::SObject_ptr where,
             CORBA::Long row );

  CORBA::Boolean IsApplicable ( const char*           theAlgoType,
                                const char*           theLibName,
                                GEOM::GEOM_Object_ptr theShapeObject,
                                CORBA::Boolean        toCheckAll);

  SMESH::long_array* GetInsideSphere( SMESH::SMESH_IDSource_ptr meshPart,
                                      SMESH::ElementType        theElemType,
                                      CORBA::Double             theX,
                                      CORBA::Double             theY,
                                      CORBA::Double             theZ,
                                      CORBA::Double             theR);

  SMESH::long_array* GetInsideBox( SMESH::SMESH_IDSource_ptr meshPart,
                                   SMESH::ElementType        theElemType,
                                   CORBA::Double             theX1,
                                   CORBA::Double             theY1,
                                   CORBA::Double             theZ1,
                                   CORBA::Double             theX2,
                                   CORBA::Double             theY2,
                                   CORBA::Double             theZ2);

  SMESH::long_array* GetInsideCylinder( SMESH::SMESH_IDSource_ptr meshPart,
                                        SMESH::ElementType        theElemType,
                                        CORBA::Double             theX,
                                        CORBA::Double             theY,
                                        CORBA::Double             theZ,
                                        CORBA::Double             theDX,
                                        CORBA::Double             theDY,
                                        CORBA::Double             theDZ,
                                        CORBA::Double             theH,
                                        CORBA::Double             theR );

  SMESH::long_array* GetInside( SMESH::SMESH_IDSource_ptr meshPart,
                                SMESH::ElementType        theElemType,
                                GEOM::GEOM_Object_ptr     theGeom,
                                CORBA::Double             theTolerance );

private:
  // Get hypothesis creator
  GenericHypothesisCreator_i* getHypothesisCreator( const char*  theHypName,
                                                    const char*  theLibName,
                                                    std::string& thePlatformLibName)
  throw ( SALOME::SALOME_Exception );
  // Create hypothesis of given type
  SMESH::SMESH_Hypothesis_ptr createHypothesis( const char* theHypName,
                                                const char* theLibName);
  // Create empty mesh on shape
  SMESH::SMESH_Mesh_ptr createMesh()
    throw ( SALOME::SALOME_Exception );

  // Check mesh icon
  bool isGeomModifIcon( SMESH::SMESH_Mesh_ptr mesh );

  // Create a sub-mesh on a geometry that is not a sub-shape of the main shape
  // for the case where a valid sub-shape not found by CopyMeshWithGeom()
  SMESH::SMESH_subMesh_ptr createInvalidSubMesh( SMESH::SMESH_Mesh_ptr mesh,
                                                 GEOM::GEOM_Object_ptr strangerGeom,
                                                 const char*           name );

  void highLightInvalid( SALOMEDS::SObject_ptr theSObject, bool isInvalid );

  SMESH::mesh_array* CreateMeshesFromMEDorSAUV( const char* theFileName,
                                                SMESH::DriverMED_ReadStatus& theStatus,
                                                const char* theCommandNameForPython,
                                                const char* theFileNameForPython);

  std::vector<long> _GetInside(SMESH::SMESH_IDSource_ptr meshPart,
                               SMESH::ElementType        ElemType,
                               const TopoDS_Shape&       Shape,
                               double*                   Tolerance = NULL);

private:
  static GEOM::GEOM_Gen_var      myGeomGen;
  static CORBA::ORB_var          myOrb;         // ORB reference
  static PortableServer::POA_var myPoa;         // POA reference
  static SALOME_NamingService*   myNS;          // Naming Service
  static SALOME_LifeCycleCORBA*  myLCC;         // Life Cycle CORBA
  static SMESH_Gen_i*            mySMESHGen;    // Point to last created instance of the class
  ::SMESH_Gen                    myGen;         // SMESH_Gen local implementation

  // hypotheses managing
  std::map<std::string, GenericHypothesisCreator_i*> myHypCreatorMap;

  StudyContext*                  myStudyContext;  // study context

  GEOM_Client*                   myShapeReader;      // Shape reader
  CORBA::Boolean                 myIsEmbeddedMode;   // Current mode
  CORBA::Boolean                 myIsEnablePublish;  // Enable publishing

  // Default color of groups
  std::string myDefaultGroupColor;

  // To load full mesh data from study at hyp modification or not
  bool myToForgetMeshDataOnHypModif;

  // Dump Python: trace of API methods calls
  Handle(TColStd_HSequenceOfAsciiString) myPythonScript;
  bool                                   myIsHistoricalPythonDump;
  std::vector< int >                     myLastParamIndex;
  std::vector< std::string >             myLastParameters;
  std::string                            myLastObj;
};


namespace SMESH
{
  template<class T>
  T
  DownCast(CORBA::Object_ptr theArg)
  {
    return dynamic_cast<T>(SMESH_Gen_i::GetServant(theArg).in());
  }

  /*!
   * \brief Function used in SMESH_CATCH to convert a caught exception to
   * SALOME::SALOME_Exception
   */
  inline void throwCorbaException(const char* excText)
  {
    THROW_SALOME_CORBA_EXCEPTION( excText, SALOME::INTERNAL_ERROR );
  }
}


#endif
