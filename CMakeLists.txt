cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(LArRecoND VERSION 01.03.00 LANGUAGES CXX)

# Automatically add the path of the linked libraries to the RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Apply the RPATH settings to the executable in the build tree, not just the installed one
# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

# Enforce out-of-source builds, a good practice from the original file
if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "LArRecoND requires an out-of-source build.")
endif()

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Default install path for ${PROJECT_NAME}" FORCE)
endif()

# Build Options
option(PANDORA_LIBTORCH "Build with LibTorch-dependent libraries" OFF)
option(PANDORA_MONITORING "Build with PandoraMonitoring support" ON)
option(LArRecoND_BUILD_DOCS "Build documentation for ${PROJECT_NAME}" OFF)

# Dependencies
# Build Options
option(PANDORA_LIBTORCH "Build with LibTorch-dependent libraries" OFF)
option(PANDORA_MONITORING "Build with PandoraMonitoring support" ON)
option(LArRecoND_BUILD_DOCS "Build documentation for ${PROJECT_NAME}" OFF)
find_package(PandoraSDK 04.00.02 REQUIRED)
find_package(LArContent 04.17.02 REQUIRED)

if(PANDORA_LIBTORCH)
    find_package(LArDLContent 04.17.02 REQUIRED)
endif()

if(PANDORA_MONITORING)
    find_package(PandoraMonitoring 04.00.02 REQUIRED)
    find_package(ROOT 6.18.04 REQUIRED COMPONENTS Eve Geom RGL EG)
endif()

if(USE_EDEPSIM)
  find_package(EDepSim)
  add_definitions("-DUSE_EDEPSIM")
endif()

# --- Executable ---
add_executable(PandoraInterface test/PandoraInterface.cxx)

file(GLOB_RECURSE LAR_RECO_ND_SRCS RELATIVE ${PROJECT_SOURCE_DIR} "src/*.cc")
add_library(${PROJECT_NAME} SHARED ${LAR_RECO_ND_SRCS})

target_include_directories(PandoraInterface PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/include)

target_compile_options(PandoraInterface PRIVATE
    -Wall
    -Wextra
    -Werror
    -pedantic
    -Wno-long-long
    -Wno-sign-compare
    -Wshadow
    -fno-strict-aliasing
    -std=c++17
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    PandoraPFA::PandoraSDK
    PandoraPFA::LArContent
)
target_link_libraries(PandoraInterface PRIVATE
    ${PROJECT_NAME}
    PandoraPFA::PandoraSDK
    PandoraPFA::LArContent
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})

if(APPLE)
    set(CMAKE_INSTALL_RPATH "@executable_path/../lib")
    set_target_properties(PandoraInterface PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}"
        INSTALL_RPATH "@executable_path/../lib"
    )
else()
    set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
    set_target_properties(PandoraInterface PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}"
        INSTALL_RPATH "$ORIGIN/../lib"
    )
endif()

if(PANDORA_LIBTORCH)
    target_link_libraries(PandoraInterface PRIVATE PandoraPFA::LArDLContent)
    target_compile_definitions(PandoraInterface PRIVATE -DLIBTORCH_DL=1)
    target_link_libraries(${PROJECT_NAME} PRIVATE PandoraPFA::LArDLContent)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DLIBTORCH_DL=1)
endif()

if(PANDORA_MONITORIN)
    if(USE_EDEPSIM)
        target_link_libraries(PandoraInterface PRIVATE
            PandoraPFA::PandoraMonitoring
            ROOT::Eve
            ROOT::Geom
            ROOT::RGL
            ROOT::EG
            EDepSim::edepsim_io
        )
    else()
        target_link_libraries(PandoraInterface PRIVATE
                PandoraPFA::PandoraMonitoring
                ROOT::Eve
                ROOT::Geom
                ROOT::RGL
                ROOT::EG
        )
    endif()
    target_compile_definitions(PandoraInterface PRIVATE -DMONITORING=1)
    target_compile_definitions(${PROJECT_NAME} PRIVATE -DMONITORING=1)
endif(

# Optional documents
if(LArRecoND_BUILD_DOCS)
    add_subdirectory(doc)
endif()

# Installation
install(TARGETS ${PROJECT_NAME} DESTINATION lib COMPONENT Runtime)
install(DIRECTORY include/ DESTINATION include COMPONENT Development FILES_MATCHING PATTERN "*.h")install(TARGETS PandoraInterface DESTINATION bin PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
