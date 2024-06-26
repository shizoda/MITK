cmake_minimum_required(VERSION 3.18 FATAL_ERROR)

# Change project and application name to your own
set(MY_PROJECT_NAME $(project-name))
set(MY_APP_NAME $(project-app-name))

#-----------------------------------------------------------------------------
# Set the language standard (MITK requires C++17)
#-----------------------------------------------------------------------------

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED 1)
set(CMAKE_CXX_EXTENSIONS 0)

#-----------------------------------------------------------------------------
# Set a default build type if none was specified
#-----------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Debug' as none was specified.")
  set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build." FORCE)

  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY
               STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

#-----------------------------------------------------------------------------
# Superbuild Option - Enabled by default
#-----------------------------------------------------------------------------

option(${MY_PROJECT_NAME}_USE_SUPERBUILD "Build ${MY_PROJECT_NAME} and the projects it depends on via SuperBuild.cmake." ON)

if(${MY_PROJECT_NAME}_USE_SUPERBUILD)
  project(${MY_PROJECT_NAME}-superbuild)
  set(${MY_PROJECT_NAME}_SOURCE_DIR ${PROJECT_SOURCE_DIR})
  set(${MY_PROJECT_NAME}_BINARY_DIR ${PROJECT_BINARY_DIR})
else()
  project(${MY_PROJECT_NAME})
endif()

set(project_policies
  CMP0001 # NEW: CMAKE_BACKWARDS_COMPATIBILITY should no longer be used.
  CMP0002 # NEW: Logical target names must be globally unique.
  CMP0003 # NEW: Libraries linked via full path no longer produce linker search paths.
  CMP0004 # NEW: Libraries linked may NOT have leading or trailing whitespace.
  CMP0005 # NEW: Preprocessor definition values are now escaped automatically.
  CMP0006 # NEW: Installing MACOSX_BUNDLE targets requires a BUNDLE DESTINATION.
  CMP0007 # NEW: List command no longer ignores empty elements.
  CMP0008 # NEW: Libraries linked by full-path must have a valid library file name.
  CMP0009 # NEW: FILE GLOB_RECURSE calls should not follow symlinks by default.
  CMP0010 # NEW: Bad variable reference syntax is an error.
  CMP0011 # NEW: Included scripts do automatic cmake_policy PUSH and POP.
  CMP0012 # NEW: if() recognizes numbers and boolean constants.
  CMP0013 # NEW: Duplicate binary directories are not allowed.
  CMP0014 # NEW: Input directories must have CMakeLists.txt
  CMP0020 # NEW: Automatically link Qt executables to qtmain target on Windows.
  CMP0028 # NEW: Double colon in target name means ALIAS or IMPORTED target.
  )
foreach(policy ${project_policies})
  if(POLICY ${policy})
    cmake_policy(SET ${policy} NEW)
  endif()
endforeach()

#-----------------------------------------------------------------------------
# Update CMake module path
#------------------------------------------------------------------------------

set(CMAKE_MODULE_PATH
  ${${MY_PROJECT_NAME}_SOURCE_DIR}/CMake
  ${CMAKE_MODULE_PATH}
  )

#-----------------------------------------------------------------------------
# CMake Function(s) and Macro(s)
#-----------------------------------------------------------------------------

include(CTest)
include(MacroEmptyExternalProject)

#-----------------------------------------------------------------------------
# Output directories.
#-----------------------------------------------------------------------------

foreach(type LIBRARY RUNTIME ARCHIVE)
  set(output_dir ${${MY_PROJECT_NAME}_BINARY_DIR}/bin)
  set(CMAKE_${type}_OUTPUT_DIRECTORY ${output_dir} CACHE INTERNAL "Single output directory for building all libraries.")
  mark_as_advanced(CMAKE_${type}_OUTPUT_DIRECTORY)
endforeach()

#-----------------------------------------------------------------------------
# Additional Options (also shown during superbuild)
#-----------------------------------------------------------------------------

option(BUILD_SHARED_LIBS "Build ${MY_PROJECT_NAME} with shared libraries" ON)
option(WITH_COVERAGE "Enable/Disable coverage" OFF)
option(BUILD_TESTING "Test the project" ON)

option(${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS "Build all ${MY_PROJECT_NAME} plugins" OFF)

mark_as_advanced(${MY_PROJECT_NAME}_INSTALL_RPATH_RELATIVE
                 ${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS
                 )

#-----------------------------------------------------------------------------
# Superbuild script
#-----------------------------------------------------------------------------

if(${MY_PROJECT_NAME}_USE_SUPERBUILD)
  include("${CMAKE_CURRENT_SOURCE_DIR}/SuperBuild.cmake")
  return()
endif()

#*****************************************************************************
#**************************** END OF SUPERBUILD ****************************
#*****************************************************************************

#-----------------------------------------------------------------------------
# Prerequesites
#-----------------------------------------------------------------------------

set(${PROJECT_NAME}_MODULES_PACKAGE_DEPENDS_DIR "${PROJECT_SOURCE_DIR}/CMake/PackageDepends")
set(MODULES_PACKAGE_DEPENDS_DIRS ${${PROJECT_NAME}_MODULES_PACKAGE_DEPENDS_DIR})

find_package(MITK 2018.04.99 REQUIRED)

if(COMMAND mitkFunctionCheckMitkCompatibility)
  mitkFunctionCheckMitkCompatibility(VERSIONS MITK_VERSION_PLUGIN_SYSTEM 1 REQUIRED)
else()
  message(SEND_ERROR "Your MITK version is too old. Please use Git hash b86bf28 or newer")
endif()

link_directories(${MITK_LINK_DIRECTORIES})

#-----------------------------------------------------------------------------
# CMake Function(s) and Macro(s)
#-----------------------------------------------------------------------------

set(CMAKE_MODULE_PATH
  ${MITK_SOURCE_DIR}/CMake
  ${CMAKE_MODULE_PATH}
  )

include(mitkFunctionCheckCompilerFlags)
include(mitkFunctionGetGccVersion)
include(mitkFunctionGetVersion)

#-----------------------------------------------------------------------------
# Set project specific options and variables (NOT available during superbuild)
#-----------------------------------------------------------------------------

set(${PROJECT_NAME}_VERSION_MAJOR "0")
set(${PROJECT_NAME}_VERSION_MINOR "1")
set(${PROJECT_NAME}_VERSION_PATCH "1")
set(${PROJECT_NAME}_VERSION_STRING "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

# Ask the user if a console window should be shown with the applications
option(${PROJECT_NAME}_SHOW_CONSOLE_WINDOW "Use this to enable or disable the console window when starting GUI Applications" ON)
mark_as_advanced(${PROJECT_NAME}_SHOW_CONSOLE_WINDOW)

if(NOT UNIX)
  set(MITK_WIN32_FORCE_STATIC "STATIC")
endif()

#-----------------------------------------------------------------------------
# Get project version info
#-----------------------------------------------------------------------------

mitkFunctionGetVersion(${PROJECT_SOURCE_DIR} ${PROJECT_NAME})

#-----------------------------------------------------------------------------
# Installation preparation
#
# These should be set before any MITK install macros are used
#-----------------------------------------------------------------------------

# on macOS all CTK plugins get copied into every
# application bundle (.app directory) specified here
set(MACOSX_BUNDLE_NAMES)
if(APPLE)
  list(APPEND MACOSX_BUNDLE_NAMES ${MY_APP_NAME})
endif(APPLE)

#-----------------------------------------------------------------------------
# Set symbol visibility Flags
#-----------------------------------------------------------------------------

if(CMAKE_COMPILER_IS_GNUCXX)
  # The MITK module build system does not yet support default hidden visibility
  set(VISIBILITY_CXX_FLAGS ) # "-fvisibility=hidden -fvisibility-inlines-hidden")
endif()

#-----------------------------------------------------------------------------
# Set coverage Flags
#-----------------------------------------------------------------------------

if(WITH_COVERAGE)
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(coverage_flags "-g -fprofile-arcs -ftest-coverage -O0 -DNDEBUG")
    set(COVERAGE_CXX_FLAGS ${coverage_flags})
    set(COVERAGE_C_FLAGS ${coverage_flags})
  endif()
endif()

#-----------------------------------------------------------------------------
# Project C/CXX Flags
#-----------------------------------------------------------------------------

set(${PROJECT_NAME}_C_FLAGS "${MITK_C_FLAGS} ${COVERAGE_C_FLAGS}")
set(${PROJECT_NAME}_C_FLAGS_DEBUG ${MITK_C_FLAGS_DEBUG})
set(${PROJECT_NAME}_C_FLAGS_RELEASE ${MITK_C_FLAGS_RELEASE})
set(${PROJECT_NAME}_CXX_FLAGS "${MITK_CXX_FLAGS} ${VISIBILITY_CXX_FLAGS} ${COVERAGE_CXX_FLAGS}")
set(${PROJECT_NAME}_CXX_FLAGS_DEBUG ${MITK_CXX_FLAGS_DEBUG})
set(${PROJECT_NAME}_CXX_FLAGS_RELEASE ${MITK_CXX_FLAGS_RELEASE})

set(${PROJECT_NAME}_EXE_LINKER_FLAGS ${MITK_EXE_LINKER_FLAGS})
set(${PROJECT_NAME}_SHARED_LINKER_FLAGS ${MITK_SHARED_LINKER_FLAGS})
set(${PROJECT_NAME}_MODULE_LINKER_FLAGS ${MITK_MODULE_LINKER_FLAGS})

#-----------------------------------------------------------------------------
# Set C/CXX Flags
#-----------------------------------------------------------------------------

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${${PROJECT_NAME}_C_FLAGS}")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${${PROJECT_NAME}_C_FLAGS_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${${PROJECT_NAME}_C_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${${PROJECT_NAME}_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${${PROJECT_NAME}_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${${PROJECT_NAME}_CXX_FLAGS_RELEASE}")

set(CMAKE_EXE_LINKER_FLAGS ${${PROJECT_NAME}_EXE_LINKER_FLAGS})
set(CMAKE_SHARED_LINKER_FLAGS ${${PROJECT_NAME}_SHARED_LINKER_FLAGS})
set(CMAKE_MODULE_LINKER_FLAGS ${${PROJECT_NAME}_MODULE_LINKER_FLAGS})

#-----------------------------------------------------------------------------
# Testing
#-----------------------------------------------------------------------------

if(BUILD_TESTING)
  # Configuration for the CMake-generated test driver
  set(CMAKE_TESTDRIVER_EXTRA_INCLUDES "#include <stdexcept>")
  set(CMAKE_TESTDRIVER_BEFORE_TESTMAIN "
    try
      {")
  set(CMAKE_TESTDRIVER_AFTER_TESTMAIN "
      }
      catch (const std::exception& e)
      {
        fprintf(stderr, \"%s\\n\", e.what());
        return EXIT_FAILURE;
      }
      catch (...)
      {
        printf(\"Exception caught in the test driver\\n\");
        return EXIT_FAILURE;
      }")
endif()

#-----------------------------------------------------------------------------
# ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR
#-----------------------------------------------------------------------------

# If ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR isn't defined, it means this project is
# *NOT* build using Superbuild. In that specific case, ${MY_PROJECT_NAME}_SUPERBUILD_BINARY_DIR
# should default to PROJECT_BINARY_DIR
if(NOT DEFINED ${PROJECT_NAME}_SUPERBUILD_BINARY_DIR)
  set(${PROJECT_NAME}_SUPERBUILD_BINARY_DIR ${PROJECT_BINARY_DIR})
endif()

#-----------------------------------------------------------------------------
# MITK modules
#-----------------------------------------------------------------------------

#add_subdirectory(Modules)

#-----------------------------------------------------------------------------
# CTK plugins
#-----------------------------------------------------------------------------

# The CMake code in this section *must* be in the top-level CMakeLists.txt file

macro(GetMyTargetLibraries all_target_libraries varname)
  set(re_ctkplugin "^$(project-plugin-base)_[a-zA-Z0-9_]+$")
  set(_tmp_list)
  list(APPEND _tmp_list ${all_target_libraries})
  ctkMacroListFilter(_tmp_list re_ctkplugin OUTPUT_VARIABLE ${varname})
endmacro()

include(${CMAKE_CURRENT_SOURCE_DIR}/Plugins/Plugins.cmake)
ctkMacroSetupPlugins(${PROJECT_PLUGINS}
                     BUILD_OPTION_PREFIX ${MY_PROJECT_NAME}_
                     BUILD_ALL ${${MY_PROJECT_NAME}_BUILD_ALL_PLUGINS})

#-----------------------------------------------------------------------------
# Add subdirectories
#-----------------------------------------------------------------------------

add_subdirectory(Apps/$(project-app-name))

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

# set MITK cpack variables
include(mitkSetupCPack)

# Customize CPack variables for this project
include(CPackSetup)

list(APPEND CPACK_CREATE_DESKTOP_LINKS "${MY_APP_NAME}")

configure_file(${MITK_SOURCE_DIR}/MITKCPackOptions.cmake.in
               ${PROJECT_BINARY_DIR}/${PROJECT_NAME}CPackOptions.cmake @ONLY)
set(CPACK_PROJECT_CONFIG_FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}CPackOptions.cmake")

# include CPack model once all variables are set
include(CPack)

# Additional installation rules
include(mitkInstallRules)

#-----------------------------------------------------------------------------
# Last configuration steps
#-----------------------------------------------------------------------------
