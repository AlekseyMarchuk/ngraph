# ******************************************************************************
# Copyright 2017-2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ******************************************************************************

# Enable ExternalProject CMake module
include(ExternalProject)

#------------------------------------------------------------------------------
# Download and install GoogleTest ...
#------------------------------------------------------------------------------

SET(TVM_GIT_REPO_URL https://github.com/dmlc/tvm.git)
SET(TVM_GIT_LABEL master)

# The 'BUILD_BYPRODUCTS' argument was introduced in CMake 3.2.
if (${CMAKE_VERSION} VERSION_LESS 3.2)
    ExternalProject_Add(
        ext_tvm
        PREFIX tvm
        GIT_REPOSITORY ${TVM_GIT_REPO_URL}
        GIT_TAG ${TVM_GIT_LABEL}
        UPDATE_COMMAND ""
        CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                   -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_ROOT}/tvm
                   -DINSTALL_DEV=ON
                   -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                   -DCMAKE_CXX_FLAGS="-fPIC"
        TMP_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/tmp"
        STAMP_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/stamp"
        DOWNLOAD_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/download"
        SOURCE_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/src"
        BINARY_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/build"
        INSTALL_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm"
        EXCLUDE_FROM_ALL TRUE
        )
else()
    ExternalProject_Add(
        ext_tvm
        PREFIX tvm
        DEPENDS ext_llvm
        GIT_REPOSITORY ${TVM_GIT_REPO_URL}
        GIT_TAG ${TVM_GIT_LABEL}
        UPDATE_COMMAND ""
        PATCH_COMMAND patch ${EXTERNAL_PROJECTS_ROOT}/tvm/src/CMakeLists.txt --forward --reject-file=- -i ${CMAKE_SOURCE_DIR}/cmake/tvm.diff || exit 0
        CMAKE_ARGS -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
                   -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER}
                   -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_ROOT}/tvm
                   -DINSTALL_DEV=ON
                   -DUSE_SORT=ON
                   -DUSE_LLVM=${EXTERNAL_PROJECTS_ROOT}/ext_llvm-prefix/src/ext_llvm/bin/llvm-config
                   -DCMAKE_CXX_FLAGS="-fPIC"
        TMP_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/tmp"
        STAMP_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/stamp"
        DOWNLOAD_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/download"
        SOURCE_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/src"
        BINARY_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/build"
        INSTALL_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm"
        BUILD_BYPRODUCTS "${EXTERNAL_PROJECTS_ROOT}/tvm/src"
        EXCLUDE_FROM_ALL TRUE
        )
endif()

#------------------------------------------------------------------------------

ExternalProject_Get_Property(ext_tvm INSTALL_DIR)
set(TVM_LINK_LIBS
    ${EXTERNAL_PROJECTS_ROOT}/tvm/build/libtvm.so
    ${EXTERNAL_PROJECTS_ROOT}/tvm/build/libtvm_topi.so
    ${EXTERNAL_PROJECTS_ROOT}/tvm/build/libtvm_runtime.so
)

# work around for set_source_files_properties in resource/CMakeLists.txt, which
# can not accept path list with spaces.
set(TVM_INCLUDE_DIR "${EXTERNAL_PROJECTS_ROOT}/tvm/include\;${EXTERNAL_PROJECTS_ROOT}/tvm/include/HalideIR")
add_library(libtvm INTERFACE)
add_dependencies(libtvm ext_tvm)
target_include_directories(libtvm SYSTEM INTERFACE
                           ${EXTERNAL_PROJECTS_ROOT}/tvm/include
                           ${EXTERNAL_PROJECTS_ROOT}/tvm/include/HalideIR)
target_link_libraries(libtvm INTERFACE ${TVM_LINK_LIBS})
