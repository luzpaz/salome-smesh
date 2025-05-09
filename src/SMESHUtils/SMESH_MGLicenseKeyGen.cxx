// Copyright (C) 2007-2025  CEA, EDF, OPEN CASCADE
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
// File      : SMESHUtils_MGLicenseKeyGen.cxx
// Created   : Sat Jul 31 18:54:16 2021
// Author    : Edward AGAPOV (OCC)

#include "SMESH_MGLicenseKeyGen.hxx"

#include "SMESH_Comment.hxx"
#include "SMESH_File.hxx"
#include "SMESH_TryCatch.hxx"

#include <Basics_DirUtils.hxx>
#include <Basics_Utils.hxx>

#include <cstdlib> // getenv, system

#include <boost/filesystem.hpp>
#include <boost/regex.hpp>
namespace boofs = boost::filesystem;

#ifdef WIN32

#  include <windows.h>
#  include <process.h>

#  define LibHandle HMODULE
#  define LoadLib( name ) LoadLibrary( name )
#  define GetProc GetProcAddress
#  define UnLoadLib( handle ) FreeLibrary( handle );

#else // WIN32

#  include <dlfcn.h>

#  define LibHandle void*
#  define LoadLib( name ) dlopen( name, RTLD_LAZY | RTLD_LOCAL )
#  define GetProc dlsym
#  define UnLoadLib( handle ) dlclose( handle );

#endif // WIN32

// to retrieve description of exception caught by SMESH_TRY
#undef SMESH_CAUGHT
#define SMESH_CAUGHT error =

constexpr char MESHGEMS_OLD_STYLE[] = "MESHGEMS_OLD_STYLE";
constexpr char SPATIAL_LICENSE[] = "SPATIAL_LICENSE";


namespace
{
  static LibHandle theLibraryHandle = nullptr; //!< handle of a loaded library

  const char* theEnvVar = "SALOME_MG_KEYGEN_LIB_PATH"; /* var specifies either full file name
                                                          of libSalomeMeshGemsKeyGenerator or
                                                          URL to download the library from */

  const char* theTmpEnvVar = "SALOME_TMP_DIR"; // directory to download the library to

  //-----------------------------------------------------------------------------------
  /*!
   * \brief Remove library file at destruction in case if it was downloaded from server
   */
  //-----------------------------------------------------------------------------------

  struct LibraryFile
  {
    std::string _name; // full file name
    bool        _isURL;

    LibraryFile(): _isURL( false ) {}

    ~LibraryFile()
    {
      if ( _isURL )
      {
        if ( theLibraryHandle )
        {
          UnLoadLib( theLibraryHandle );
          theLibraryHandle = nullptr;
        }

        std::string tmpDir; // tmp dir that should not be removed
        if ( const char* libPath = getenv( theTmpEnvVar ))
        {
          tmpDir = libPath;
          while (( !tmpDir.empty() ) &&
                 ( tmpDir.back() == '/' || tmpDir.back() == '\\' ))
            tmpDir.pop_back();
        }

        while ( SMESH_File( _name ).remove() )
        {
          size_t length = _name.size();
          _name = boofs::path( _name ).parent_path().string(); // goto parent directory
          if ( _name.size() == length )
            break; // no more parents

          if ( _name == tmpDir )
            break; // don't remove tmp dir

          if ( !Kernel_Utils::IsEmptyDir( _name ))
            break;
        }
      }
    }
  };


  //================================================================================
  /*!
   * \brief Retrieve description of the last error
   *  \param [out] error - return the description
   *  \return bool - true if the description found
   */
  //================================================================================

  bool getLastError( std::string& error )
  {
#ifndef WIN32

    if ( const char* text = dlerror() )
    {
      error = text;
      return true;
    }
    return false;

#else

    DWORD dw = GetLastError();
    void* cstr;
    DWORD msgLen = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                 NULL,
                                 dw,
                                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
                                 (LPTSTR) &cstr,
                                 0,
                                 NULL
                                 );
    if ( msgLen > 0 ) {
#  if defined( UNICODE )
      error = Kernel_Utils::encode_s((wchar_t*)cstr);
#  else
      error = (char*)cstr;
#  endif
      LocalFree(cstr);
    }

    return (bool)msgLen;

#endif
  }

  //================================================================================
  /*!
   * \brief Adjust file extension according to the platform
   */
  //================================================================================

  bool setExtension( std::string& fileName, std::string& error )
  {
    if ( fileName.empty() )
    {
      error = "Library file name is empty";
      return false;
    }
#if defined(WIN32)
    std::string ext = ".dll";
#elif defined(__APPLE__)
    std::string ext = ".dylib";
#else
    std::string ext = ".so";
#endif

    fileName = fileName.substr( 0, fileName.find_last_of('.')) + ext;
    return true;
  }

  //================================================================================
  /*!
   * \brief Check if library file name looks like an URL
   *  \param [in,out] libraryFile - holds file name and returns result in _isURL member field
   *  \return bool - true if the file name looks like an URL
   */
  //================================================================================

  bool isURL( LibraryFile & libraryFile )
  {
    {// round1
      enum { SCHEME = 2, AUTHORITY = 4, PATH = 5 }; // sub-strings
      boost::regex urlRegex ( R"(^(([^:\/?#]+):)?(//([^\/?#]*))?([^?#]*)(\?([^#]*))?(#(.*))?)",
                              boost::regex::extended );
      boost::smatch matchResult;

      libraryFile._isURL = false;
      if ( boost::regex_match( libraryFile._name, matchResult, urlRegex ))
        libraryFile._isURL = ( !matchResult.str( SCHEME    ).empty() &&
                              !matchResult.str( AUTHORITY ).empty() &&
                              !matchResult.str( PATH      ).empty() );
    }
    if(libraryFile._isURL)
      return true;
    {// round2
      enum { HOST = 2, PORT = 3, PATH = 4 }; // sub-strings
      boost::regex urlRegex ( R"(^(([^:\/?#]+):)?([^/]+)?(/[^#]*))",
                              boost::regex::extended );
      boost::smatch matchResult;

      libraryFile._isURL = false;
      if ( boost::regex_match( libraryFile._name, matchResult, urlRegex ))
        libraryFile._isURL = ( !matchResult.str( HOST ).empty() &&
                              !matchResult.str( PORT ).empty() &&
                              !matchResult.str( PATH ).empty() );
    }
    return libraryFile._isURL;
  }

  //================================================================================
  /*!
   * \brief Download libraryFile._name URL to SALOME_TMP_DIR
   *  \param [in,out] libraryFile - holds the URL and returns name of a downloaded file
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================

  bool downloadLib( LibraryFile& libraryFile, std::string & error )
  {
    // check if can write into SALOME_TMP_DIR

    std::string tmpDir = Kernel_Utils::GetTmpDirByEnv( theTmpEnvVar );
    if ( tmpDir.empty() ||
         !Kernel_Utils::IsExists( tmpDir ))
    {
      error = "Can't download " + libraryFile._name + " as SALOME_TMP_DIR is not correctly set";
      return false;
    }
    if ( !Kernel_Utils::IsWritable( tmpDir ))
    {
      error = "Can't download " + libraryFile._name + " as '" + tmpDir + "' is not writable. "
        "Check SALOME_TMP_DIR environment variable";
      return false;
    }

    // Download

    std::string url = libraryFile._name;

#ifdef WIN32

    std::string outFile = tmpDir + "MeshGemsKeyGenerator.dll";

    // use wget (== Invoke-WebRequest) PowerShell command available since Windows 7
    std::string psCmd = "wget -Uri " + url + " -OutFile " + outFile;
    std::string   cmd = "powershell.exe " + psCmd;

#else

    std::string outFile = tmpDir + "libMeshGemsKeyGenerator.so";

    std::string cmd = "smesh_wget.py " + url + " -O " + outFile;

#endif

    if ( Kernel_Utils::IsExists( outFile )) // remove existing file
    {
      SMESH_File lib( outFile, /*open=*/false );
      if ( !lib.remove() )
      {
        error = lib.error();
        return false;
      }
    }

#ifndef WIN32
    //[EDF25906]
    std::string redirect = tmpDir + "redirect.out";
    std::ostringstream oss;
    oss << cmd << " " << redirect;
    cmd = oss.str();
#endif

    system( cmd.c_str() ); // download

#ifndef WIN32
    {//[EDF25906]
      std::ifstream infile(redirect);
      infile.seekg(0, std::ios::end);
      size_t length = infile.tellg();
      infile.seekg(0, std::ios::beg);
      std::unique_ptr<char []> buffer(new char[length+1]);
      buffer[length] = '\0';
      infile.read(const_cast<char *>( buffer.get() ),length);

      MESSAGE( buffer.get() );
    }
    {
      SMESH_File redirectFile( redirect, /*open=*/false );
      redirectFile.remove();
    }
#endif

    SMESH_File resultFile( outFile, /*open=*/false );
    bool ok = ( resultFile.exists() && resultFile.size() > 0 );

    if ( ok )
      libraryFile._name = outFile;
    else
      error = "Can't download file " + url;

    return ok;
  }

  //================================================================================
  /*!
   * \brief Load libMeshGemsKeyGenerator.so
   *  \param [out] error - return error description
   *  \param [out] libraryFile - return library file name and _isURL flag
   *  \return bool - is a success
   */
  //================================================================================

  bool loadLibrary( std::string& error, LibraryFile& libraryFile )
  {
    if ( theLibraryHandle )
      return true;

    const char* libPath = getenv( theEnvVar );
    if ( !libPath )
    {
      error = SMESH_Comment( "Environment variable ") <<  theEnvVar << " is not set";
      return false;
    }

    libraryFile._name = libPath;
    // if ( !setExtension( libraryFile._name, error )) // is it necessary?
    //   return false;

    if ( isURL( libraryFile ))
    {
      if ( !downloadLib( libraryFile, error ))
      {
        // try to fix extension
        std::string url = libraryFile._name;
        if ( !setExtension( libraryFile._name, error ))
          return false;
        if ( url == libraryFile._name )
          return false; // extension not changed

        if ( !downloadLib( libraryFile, error ))
          return false;
      }
    }

#if defined( WIN32 ) && defined( UNICODE )
    std::wstring encodePath = Kernel_Utils::utf8_decode_s( libraryFile._name );
    const wchar_t*     path = encodePath.c_str();
#else
    const char*        path = libraryFile._name.c_str();
#endif

    theLibraryHandle = LoadLib( path );
    if ( !theLibraryHandle )
    {
      if ( ! getLastError( error ))
        error = "Can't load library '" + libraryFile._name + "'";
    }

    return theLibraryHandle;
  }

} // anonymous namespace


namespace SMESHUtils_MGLicenseKeyGen // API implementation
{
  //================================================================================
  /*!
   * \brief Sign a CAD
   *  \param [in] meshgems_cad - pointer to a MG CAD object (meshgems_cad_t)
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================

  bool SignCAD_After( void* meshgems_cad, std::string& error )
  {
    LibraryFile libraryFile;
    if ( !loadLibrary( error, libraryFile ))
      return false;

    bool ok = false;
    typedef bool (*SignFun)(void* );
    SignFun signFun = (SignFun) GetProc( theLibraryHandle, "SignCAD" );
    if ( !signFun )
    {
      if ( ! getLastError( error ))
        error = SMESH_Comment( "Can't find symbol 'SignCAD' in '") << getenv( theEnvVar ) << "'";
    }
    else
    {
      SMESH_TRY;

      ok = signFun( meshgems_cad );

      SMESH_CATCH( SMESH::returnError );

      if ( !error.empty() )
        ok = false;
      else if ( !ok )
        error = "SignCAD() failed (located in '" + libraryFile._name + "')";
    }
    return ok;
  }

  //================================================================================
  /*!
   * \brief Unlock a specific MeshGems product (for products called as a library)
   *  \param [in] product - product of MeshGems to unlock
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================
  bool UnlockProduct( const std::string& product, std::string& error )
  {
    MESSAGE("SMESH UnlockProduct: " << product);
    LibraryFile libraryFile;
    if ( !loadLibrary( error, libraryFile ))
      return false;

    bool ok = false;
    typedef bool (*SignFun)(const std::string& );

    // specific function to unlock each product
    std::string function = "UnlockProduct";

    SignFun signFun = (SignFun) GetProc( theLibraryHandle, function.c_str() );
    if ( !signFun )
    {
      if ( ! getLastError( error ))
        error = SMESH_Comment( "Can't find symbol '") << function << "' in '" << getenv( theEnvVar ) << "'";
    }
    else
    {
      SMESH_TRY;

      ok = signFun( product.c_str() );

      SMESH_CATCH( SMESH::returnError );

      if ( !error.empty() )
      {
        std::cerr << "error: " << error << std::endl;
        ok = false;
      }
      else if ( !ok )
        error = "UnlockProduct() failed (located in '" + libraryFile._name + "')";
    }
    return ok;
  }

  //================================================================================
  /*!
   * \brief Sign a CAD (or don't do it if env MESHGEMS_OLD_STYLE is set)
   *  \param [in] meshgems_cad - pointer to a MG CAD object (meshgems_cad_t)
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================
  bool SignCAD( void* meshgems_cad, std::string& error )
  {
    const char *meshGemsOldStyleEnvVar( getenv( MESHGEMS_OLD_STYLE ) );
    if ( !meshGemsOldStyleEnvVar || strlen(meshGemsOldStyleEnvVar) == 0 )
    {
      if (NeedsMGSpatialEnvLicense(error))
        // SignCAD is only called by cadsurf. Other components call SignMesh
        return UnlockProduct("cadsurf", error);
      else
        return SignCAD_After(meshgems_cad, error);
    }
    else
      return true;
  }

  //================================================================================
  /*!
   * \brief Sign a mesh
   *  \param [in] meshgems_mesh - pointer to a MG mesh (meshgems_mesh_t)
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================

  bool SignMesh_After( void* meshgems_mesh, std::string& error )
  {
    LibraryFile libraryFile;
    if ( !loadLibrary( error, libraryFile ))
      return false;

    bool ok = false;
    typedef bool (*SignFun)(void* );
    SignFun signFun = (SignFun) GetProc( theLibraryHandle, "SignMesh" );
    if ( !signFun )
    {
      if ( ! getLastError( error ))
        error = SMESH_Comment( "Can't find symbol 'SignMesh' in '") << getenv( theEnvVar ) << "'";
    }
    else
    {
      SMESH_TRY;

      ok = signFun( meshgems_mesh );

      SMESH_CATCH( SMESH::returnError );

      if ( !error.empty() )
        ok = false;
      else if ( !ok )
        error = "SignMesh() failed (located in '" + libraryFile._name + "')";
    }
    return ok;
  }

  //================================================================================
  /*!
   * \brief Sign a mesh (or don't do it if env MESHGEMS_OLD_STYLE is set)
   *  \param [in] meshgems_mesh - pointer to a MG mesh (meshgems_mesh_t)
   *  \param [in] product - product of MeshGems to unlock
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================
  bool SignMesh( void* meshgems_mesh, const std::string& product, std::string& error )
  {
    const char *meshGemsOldStyleEnvVar( getenv( MESHGEMS_OLD_STYLE ) );
    if ( !meshGemsOldStyleEnvVar || strlen(meshGemsOldStyleEnvVar) == 0 )
    {
      if (NeedsMGSpatialEnvLicense(error))
        // unlock product (MG 2.15)
        return UnlockProduct(product, error);
      else
        // sign the mesh (MG 2.13 and 2.14)
        return SignMesh_After(meshgems_mesh, error);
    }
    else
      // use DLIM8 server (nothing to do here)
      return true;
  }

  //================================================================================
  /*!
   * \brief Return a license key to pass as argument to a MG mesher executable
   *  \param [in] gmfFile - path to an input mesh file
   *  \param [in] nb* - nb of entities in the input mesh
   *  \param [out] error - return error description
   *  \return std::string - the key
   */
  //================================================================================

  std::string GetKey_After(const std::string& gmfFile,
                           int                nbVertex,
                           int                nbEdge,
                           int                nbFace,
                           int                nbVol,
                           std::string&       error)
  {
    std::string key;
    LibraryFile libraryFile;
    if ( !loadLibrary( error, libraryFile ))
      return key;

    typedef std::string (*GetKeyFun)(std::string const &, int, int, int, int );
    GetKeyFun keyFun = (GetKeyFun) GetProc( theLibraryHandle, "GetKey" );
    if ( !keyFun )
    {
      if ( ! getLastError( error ))
        error = SMESH_Comment( "Can't find symbol 'GetKey' in '") << getenv( theEnvVar ) << "'";
    }
    else
    {
      key = keyFun( gmfFile, nbVertex, nbEdge, nbFace, nbVol );
    }
    if ( key.empty() )
      error = "GetKey() failed (located in '" + libraryFile._name + "')";

    return key;
  }

  //================================================================================
  /*!
   * \brief Return a license key to pass as argument to a MG mesher executable (>2.15)
   *  \param [out] error - return error description
   *  \return std::string - the key
   */
  //================================================================================

  std::string GetKey_After(std::string&       error)
  {
    std::string key;
    LibraryFile libraryFile;
    if ( !loadLibrary( error, libraryFile ))
      return key;

    typedef std::string (*GetKeyFun)();
    GetKeyFun keyFun = (GetKeyFun) GetProc( theLibraryHandle, "GetKey" );
    if ( !keyFun )
    {
      if ( ! getLastError( error ))
        error = SMESH_Comment( "Can't find symbol 'GetKey' in '") << getenv( theEnvVar ) << "'";
    }
    else
    {
      key = keyFun( );
    }
    if ( key.empty() )
      error = "GetKey() failed (located in '" + libraryFile._name + "')";

    return key;
  }

  //================================================================================
  /*!
   * \brief Get MeshGems version major/minor/patch from the environment variables
   *  \param [out] error - return error description
   *  \return int - the version
   */
  //================================================================================
  int GetMGVersionFromEnv(const char* env_variable)
  {
    MESSAGE("Entering GetMGVersionFromEnv and calling " << env_variable);
    int version = -1;
    if (getenv(env_variable) == nullptr )
    {
      MESSAGE("Could not find " << env_variable << " from environment");
    }
    else
    {
      version = std::stoi(std::string(getenv(env_variable)));
    }
    return version;
  }
  //================================================================================
  /*!
   * \brief Get MeshGems version major/minor/patch from the keygen library and meshgems built-in functions
   *  \param [out] error - return error description
   *  \return int - the function implemented in the library
   */
  //================================================================================
  int GetMGVersionFromFunction(const char* function_name)
  {
    MESSAGE("Entering GetMGVersionFromFunction and calling " << function_name);
    int version = -1;
    typedef int (*GetKeyFun)();
    GetKeyFun keyFun = (GetKeyFun) GetProc( theLibraryHandle, function_name);
    if ( !keyFun )
    {
      MESSAGE("Could not find " << function_name << " from library");
    }
    else
    {
      version = keyFun();
    }
    return version;
  }

  //================================================================================
  /*!
   * \brief Get MeshGems version from the keygen library or meshgems built-in functions
   *  \param [out] error - return error description
   *  \return int - the version
   */
  //================================================================================
  int GetMGVersionHex(std::string&       error)
  {
    // load mgkeygen library
    int v_min = -1;
    LibraryFile libraryFile;
    if ( !loadLibrary( error, libraryFile ))
      return v_min;
    MESSAGE("Extracting MeshGems version");

    // get minor version
    v_min = GetMGVersionFromFunction("meshgems_core_get_version_minor");
    if (v_min == -1)
      v_min = GetMGVersionFromFunction("GetVersionMinor");
    if (v_min == -1)
      v_min = GetMGVersionFromEnv("MESHGEMS_VERSION_MINOR");
    if (v_min == -1)
      error = "could not retrieve minor version (located in '" + libraryFile._name + "')";
    MESSAGE("MeshGems minor version =  " << v_min);

    // get major version
    int v_maj = GetMGVersionFromFunction("meshgems_core_get_version_major");
    if (v_maj == -1)
      v_maj = GetMGVersionFromFunction("GetVersionMajor");
    if (v_maj == -1)
      v_maj = GetMGVersionFromEnv("MESHGEMS_VERSION_MAJOR");
    if (v_maj == -1)
      error = "could not retrieve major version (located in '" + libraryFile._name + "')";
    MESSAGE("MeshGems major version = " << v_maj);

    // get patch version
    int v_patch = GetMGVersionFromFunction("meshgems_core_get_version_patch");
    if (v_patch == -1)
      v_patch = GetMGVersionFromFunction("GetVersionPatch");
    if (v_patch == -1)
      v_patch = GetMGVersionFromEnv("MESHGEMS_VERSION_PATCH");
    if (v_patch == -1)
      error = "could not retrieve patch version (located in '" + libraryFile._name + "')";
    MESSAGE("MeshGems patch version = " << v_patch);

    int v_hex = (v_maj << 16 | v_min << 8 | v_patch);
    MESSAGE("v_hex: " << v_hex);

    return v_hex;
  }

  //================================================================================
  /*!
   * \brief Guess if the Spatial license is needed (if MeshGems is > 2.15.0)
   *  \param [out] error - return error description
   *  \return bool - true if MeshGems is > 2.15.0
   */
  //================================================================================
  bool NeedsMGSpatialEnvLicense(std::string& error)
  {
    // if MeshGems version is > 2.15.0, need to set SPATIAL_LICENSE
    int v_hex = GetMGVersionHex(error);
    bool ok = (v_hex > MESHGEMS_215);
    if (ok)
      MESSAGE("MeshGems version is > 2.15.0, need to set SPATIAL_LICENSE");
    return ok;
  }

  //================================================================================
  /*!
   * \brief Set the SPATIAL_LICENSE environment variable
   *  \param [out] error - return error description
   *  \return bool - true in case of success
   */
  //================================================================================
  bool SetMGSpatialEnvLicense(std::string& error)
  {
    int ok;
    std::string key = GetKey(error);
#ifndef WIN32
    ok = setenv(SPATIAL_LICENSE, key.c_str(), 0); // 0 means do not overwrite
#else
    ok = Kernel_Utils::setenv(SPATIAL_LICENSE, key.c_str(), 0 );
#endif
    MESSAGE("Set SPATIAL_LICENSE");
    return (ok==0);
  }

  //================================================================================
  /*!
   * \brief Get the license key from libMeshGemsKeyGenerator.so or $SPATIAL_LICENSE
   * Called by plugins calling MG products as executables.
   * If MESHGEMS_OLD_STYLE is set, return "0", to use old DLIM8 server license
   * instead of the key.
   *  \param [in] gmfFile - path to an input mesh file
   *  \param [in] nb* - nb of entities in the input mesh
   *  \param [out] error - return error description
   *  \return std::string - the key
   */
  //================================================================================
  std::string GetKey(const std::string& gmfFile,
                     int                nbVertex,
                     int                nbEdge,
                     int                nbFace,
                     int                nbVol,
                     std::string&       error)
  {
    // default key if MESHGEMS_OLD_STYLE or SPATIAL_LICENSE is set
    std::string key("0");
    const char *meshGemsOldStyleEnvVar( getenv( MESHGEMS_OLD_STYLE ) );
    if ( !meshGemsOldStyleEnvVar || strlen(meshGemsOldStyleEnvVar) == 0 )
    {
      const char *spatialLicenseEnvVar( getenv( SPATIAL_LICENSE ) );
      if ( !spatialLicenseEnvVar || strlen(spatialLicenseEnvVar) == 0 )
      {
        if (NeedsMGSpatialEnvLicense(error))
        {
          // if MG version > 2.15, set environment license, don't return it as a key
          // otherwise it will be printed in the command line
          MESSAGE("SPATIAL_LICENSE not in env => we add it from MGKeygen .so");
          SetMGSpatialEnvLicense(error);
        }
        else
        {
          // generate the key from the mesh info (MG 2.13 and 2.14)
          MESSAGE("MG < 2.15 => get the key from MGKeygen .so and this mesh info");
          key = GetKey_After(gmfFile,nbVertex,nbEdge,nbFace,nbVol,error);
        }
      }
      else
        MESSAGE("SPATIAL_LICENSE already in env => we use it");
    }
    if (! error.empty())
      std::cerr << error;
    return key;
  }

  //================================================================================
  /*!
   * \brief Get the license key from libMeshGemsKeyGenerator.so or $SPATIAL_LICENSE
   * Called for MG 2.15 by CADSurf and MG plugins calling MG products as library,
   * i.e. compiled as library with -DSALOME_USE_MG_LIBS=ON
   *  \param [out] error - return error description
   *  \return std::string - the key
   */
  //================================================================================
  std::string GetKey(std::string&       error)
  {
    // default key if not found in .so or in SPATIAL_LICENSE
    std::string key("0");
    const char *meshGemsOldStyleEnvVar( getenv( MESHGEMS_OLD_STYLE ) );
    if ( !meshGemsOldStyleEnvVar || strlen(meshGemsOldStyleEnvVar) == 0 ){
      const char *spatialLicenseEnvVar( getenv( SPATIAL_LICENSE ) );
      if ( !spatialLicenseEnvVar || strlen(spatialLicenseEnvVar) == 0 )
      {
        MESSAGE("SPATIAL_LICENSE not in env => we add it from MGKeygen .so");
        // use new style, i.e. key in a library
        key = GetKey_After(error);
      }
      else
      {
        MESSAGE("SPATIAL_LICENSE already in env => we use it");
        key = std::string(spatialLicenseEnvVar);
      }
    }
    if (! error.empty())
      std::cerr << error;
    return key;
  }

  //================================================================================
  /*!
   * \brief Return false if libMeshGemsKeyGenerator.so is not functional
   *  \param [out] error - return error description
   *  \return bool - is a success
   */
  //================================================================================

  bool CheckKeyGenLibrary( std::string& error )
  {
    return !GetKey("",4,0,2,0,error ).empty();
  }

  //================================================================================
  /*!
   * \brief Return KeyGenerator library name
   */
  //================================================================================

  std::string GetLibraryName()
  {
    std::string libName, error;
    if ( const char* libPath = getenv( theEnvVar ))
    {
      libName = Kernel_Utils::GetBaseName( libPath );
    }
    else
    {
      libName = "libSalomeMeshGemsKeyGenerator";
    }
    setExtension( libName, error );
    return libName;
  }
}
