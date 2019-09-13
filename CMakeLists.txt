# CMakeLists.txt for <X265EncoderFilter> 
cmake_minimum_required(VERSION 3.11)

# hints from https://github.com/robotology/how-to-export-cpp-library/blob/master/CMakeLists.txt
project(X265EncoderFilter
        LANGUAGES CXX C
        VERSION 0.0.0.0)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_CRT_SECURE_NO_WARNINGS")

include(FetchContent)

FetchContent_Declare(
  DirectShowExt
  GIT_REPOSITORY https://github.com/CSIR-RTVC/DirectShowExt
)
# Declare dependencies
find_package(DirectShowExt 1.0.0 REQUIRED)

FetchContent_Declare(
  X265v2
  GIT_REPOSITORY https://github.com/CSIR-RTVC/X265v2
)
# Declare dependencies
find_package(X265v2 1.0.0 REQUIRED)

FetchContent_Declare(
  Vpp
  GIT_REPOSITORY https://github.com/CSIR-RTVC/vpp
)
# Declare dependencies
find_package(Vpp 1.0.0 REQUIRED)

SET(FLT_HDRS
X265EncoderFilter.h
X265EncoderProperties.h
resource.h
stdafx.h
VersionInfo.h
)

SET(FLT_SRCS 
DLLSetup.cpp
X265EncoderFilter.cpp
X265EncoderFilter.def
X265EncoderFilter.rc
stdafx.cpp
)

ADD_LIBRARY(
X265EncoderFilter SHARED ${FLT_SRCS} ${FLT_HDRS})

target_include_directories(X265EncoderFilter
    PUBLIC 
        $<INSTALL_INTERFACE:install>    
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

LINK_DIRECTORIES(
    $ENV{MSSDK}/Lib
)

TARGET_LINK_LIBRARIES (
X265EncoderFilter
DirectShowExt::DirectShowExt
Vpp::Vpp
X265v2::X265v2
) 

INSTALL(
  TARGETS X265EncoderFilter
  ARCHIVE DESTINATION bin
  LIBRARY DESTINATION bin
)

IF (REGISTER_DS_FILTERS)
ADD_CUSTOM_COMMAND(
TARGET X265EncoderFilter 
POST_BUILD COMMAND 
regsvr32 /s \"$(TargetPath)\"
)
ENDIF(REGISTER_DS_FILTERS)
