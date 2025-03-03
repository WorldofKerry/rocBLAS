# ########################################################################
# Copyright (C) 2018-2023 Advanced Micro Devices, Inc. All rights reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell cop-
# ies of the Software, and to permit persons to whom the Software is furnished
# to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IM-
# PLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNE-
# CTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
# ########################################################################

#enable_testing()
find_package( GTest REQUIRED )

if( BUILD_WITH_TENSILE )
  set(rocblas_tensile_test_source
      # Keep multiheaded_gtest.cpp first as we want
      # to allow it to create the first TensileHost !
      # Current GTESTs are run in the linking order as added with global variables
      multiheaded_gtest.cpp
      # use of tensile based functions (gemm)
      atomics_mode_gtest.cpp
      get_solutions_gtest.cpp

  )
endif()

set(rocblas_no_tensile_test_source
    # general
    rocblas_gtest_main.cpp
    rocblas_test.cpp
    general_gtest.cpp
    set_get_pointer_mode_gtest.cpp
    set_get_atomics_mode_gtest.cpp
    logging_mode_gtest.cpp
    ostream_threadsafety_gtest.cpp
    set_get_vector_gtest.cpp
    set_get_matrix_gtest.cpp
    # blas1
    blas1/asum_gtest.cpp
    blas1/axpy_gtest.cpp
    blas1/copy_gtest.cpp
    blas1/dot_gtest.cpp
    blas1/iamaxmin_gtest.cpp
    blas1/nrm2_gtest.cpp
    blas1/rot_gtest.cpp
    blas1/scal_gtest.cpp
    blas1/swap_gtest.cpp
    # blas1_ex
    blas_ex/axpy_ex_gtest.cpp
    blas_ex/dot_ex_gtest.cpp
    blas_ex/nrm2_ex_gtest.cpp
    blas_ex/rot_ex_gtest.cpp
    blas_ex/scal_ex_gtest.cpp
    # blas2
    blas2/trsv_gtest.cpp
    blas2/gbmv_gtest.cpp
    blas2/gemv_gtest.cpp
    blas2/hbmv_gtest.cpp
    blas2/hemv_gtest.cpp
    blas2/her_gtest.cpp
    blas2/her2_gtest.cpp
    blas2/hpmv_gtest.cpp
    blas2/hpr_gtest.cpp
    blas2/hpr2_gtest.cpp
    blas2/trmv_gtest.cpp
    blas2/tpmv_gtest.cpp
    blas2/tbmv_gtest.cpp
    blas2/tbsv_gtest.cpp
    blas2/tpsv_gtest.cpp
    blas2/ger_gtest.cpp
    blas2/geru_gtest.cpp
    blas2/gerc_gtest.cpp
    blas2/spr_gtest.cpp
    blas2/spr2_gtest.cpp
    blas2/syr_gtest.cpp
    blas2/syr2_gtest.cpp
    blas2/sbmv_gtest.cpp
    blas2/spmv_gtest.cpp
    blas2/symv_gtest.cpp
    # blas3 may use tensile or source gemm
    blas3/gemm_gtest.cpp
    blas3/symm_gtest.cpp
    blas3/hemm_gtest.cpp
    blas3/trsm_gtest.cpp
    blas3/trtri_gtest.cpp
    blas3/trmm_gtest.cpp
    blas3/syrk_gtest.cpp
    blas3/syrkx_gtest.cpp
    blas3/syr2k_gtest.cpp
    blas3/herk_gtest.cpp
    blas3/herkx_gtest.cpp
    blas3/her2k_gtest.cpp
    blas3/dgmm_gtest.cpp
    blas3/geam_gtest.cpp
    # blas3_ex
    blas_ex/gemmt_gtest.cpp
    blas_ex/geam_ex_gtest.cpp
    blas_ex/gemm_ex3_gtest.cpp
  )

# Keep ${rocblas_tensile_test_source} first, so that multiheaded tests are the
# first to initialize Tensile.
if ( BUILD_FORTRAN_CLIENTS )
  add_executable( rocblas-test ${rocblas_tensile_test_source} ${rocblas_no_tensile_test_source} $<TARGET_OBJECTS:rocblas_fortran> ${rocblas_test_bench_common} )
else()
  add_executable( rocblas-test ${rocblas_tensile_test_source} ${rocblas_no_tensile_test_source} ${rocblas_test_bench_common} )
endif()

if (WIN32)
  target_compile_definitions( rocblas-test PRIVATE _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING )
endif()

target_compile_definitions( rocblas-test PRIVATE ${TENSILE_DEFINES} GOOGLE_TEST)
if ( NOT BUILD_FORTRAN_CLIENTS )
  target_compile_definitions( rocblas-test PRIVATE CLIENTS_NO_FORTRAN )
endif()

# Internal header includes
target_include_directories( rocblas-test
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/blas1>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/blas2>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/blas3>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include/blas_ex>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/blas_ex>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../library/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../library/src/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../library/src>
)

# External header includes included as system files
target_include_directories( rocblas-test
  SYSTEM PRIVATE
    $<BUILD_INTERFACE:${HIP_INCLUDE_DIRS}>
    $<BUILD_INTERFACE:${BLAS_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${BLIS_INCLUDE_DIR}> # may be blank if not used
    $<BUILD_INTERFACE:${GTEST_INCLUDE_DIRS}>
)

if( BUILD_FORTRAN_CLIENTS )
  target_link_libraries( rocblas-test PRIVATE rocblas_fortran_client )
endif( )
target_link_libraries( rocblas-test PRIVATE ${BLAS_LIBRARY} ${GTEST_BOTH_LIBRARIES} roc::rocblas )

if( CUDA_FOUND )
  target_include_directories( rocblas-test
    PRIVATE
      $<BUILD_INTERFACE:${CUDA_INCLUDE_DIRS}>
      $<BUILD_INTERFACE:${hip_INCLUDE_DIRS}>
    )
  target_compile_definitions( rocblas-test PRIVATE __HIP_PLATFORM_NVCC__ )
  target_link_libraries( rocblas-test PRIVATE ${CUDA_LIBRARIES} )
else( )
  # auto set in hip_common.h
  #target_compile_definitions( rocblas-test PRIVATE __HIP_PLATFORM_HCC__ )
  target_link_libraries( rocblas-test PRIVATE hip::host hip::device )
endif( )

if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  # GCC or hip-clang needs specific flag to turn on f16c intrinsics
  target_compile_options( rocblas-test PRIVATE -mf16c )
endif( )

target_compile_definitions( rocblas-test PRIVATE ROCM_USE_FLOAT16 ROCBLAS_INTERNAL_API ROCBLAS_NO_DEPRECATED_WARNINGS )

target_compile_options(rocblas-test PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${COMMON_CXX_OPTIONS}>)
# target_compile_options does not go to linker like CMAKE_CXX_FLAGS does, so manually add
if (NOT WIN32)
  list( APPEND COMMON_LINK_LIBS "-lm -lstdc++fs")
  if (NOT BUILD_FORTRAN_CLIENTS)
    list( APPEND COMMON_LINK_LIBS "-lgfortran") # for lapack
  endif()
else()
  list( APPEND COMMON_LINK_LIBS "libomp")
endif()
target_link_libraries( rocblas-test PRIVATE ${COMMON_LINK_LIBS} )

if (WIN32)
  # for now adding in all .dll as dependency chain is not cmake based on win32
  file( GLOB third_party_dlls
    LIST_DIRECTORIES OFF
    CONFIGURE_DEPENDS
    ${OPENBLAS_DIR}/bin/*.dll
    ${HIP_DIR}/bin/amd*.dll
    ${HIP_DIR}/bin/hiprt*.dll
    ${HIP_DIR}/bin/hipinfo.exe
    ${CMAKE_SOURCE_DIR}/rtest.*
    C:/Windows/System32/libomp140*.dll
  )
  foreach( file_i ${third_party_dlls})
    add_custom_command( TARGET rocblas-test POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy ${file_i} ${PROJECT_BINARY_DIR}/staging/ )
  endforeach( file_i )
endif()

set_target_properties( rocblas-test PROPERTIES
  IMPORT_PREFIX ""
  IMPORT_SUFFIX ".lib"
  LINKER_LANGUAGE CXX
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/staging"
)

set( ROCBLAS_TEST_DATA "${PROJECT_BINARY_DIR}/staging/rocblas_gtest.data")
add_custom_command( OUTPUT "${ROCBLAS_TEST_DATA}"
                    COMMAND ${python} ../common/rocblas_gentest.py -I ../include rocblas_gtest.yaml -o "${ROCBLAS_TEST_DATA}"
                    DEPENDS ../common/rocblas_gentest.py ../include/rocblas_common.yaml general_gtest.yaml blas1_gtest.yaml dgmm_gtest.yaml gbmv_gtest.yaml geam_gtest.yaml geam_ex_gtest.yaml gemm_batched_gtest.yaml gemm_gtest.yaml gemm_strided_batched_gtest.yaml gemmt_gtest.yaml gemv_gtest.yaml ger_gtest.yaml geruc_gtest.yaml hbmv_gtest.yaml hemm_gtest.yaml hemv_gtest.yaml her2_gtest.yaml her2k_gtest.yaml her_gtest.yaml herk_gtest.yaml herkx_gtest.yaml hpmv_gtest.yaml hpr2_gtest.yaml hpr_gtest.yaml known_bugs.yaml logging_mode_gtest.yaml atomics_mode_gtest.yaml ostream_threadsafety_gtest.yaml rocblas_gtest.yaml sbmv_gtest.yaml set_get_matrix_gtest.yaml set_get_pointer_mode_gtest.yaml set_get_atomics_mode_gtest.yaml set_get_vector_gtest.yaml spmv_gtest.yaml spr2_gtest.yaml spr_gtest.yaml symm_gtest.yaml symv_gtest.yaml syr2_gtest.yaml syr2k_gtest.yaml syr_gtest.yaml syrk_gtest.yaml syrkx_gtest.yaml tbmv_gtest.yaml tbsv_gtest.yaml tpmv_gtest.yaml tpsv_gtest.yaml trmm_gtest.yaml trmv_gtest.yaml trsm_gtest.yaml trsv_gtest.yaml trtri_gtest.yaml multiheaded_gtest.yaml get_solutions_gtest.yaml
                    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" )
add_custom_target( rocblas-test-data DEPENDS "${ROCBLAS_TEST_DATA}" )

add_dependencies( rocblas-test rocblas-test-data rocblas-common )

rocm_install(TARGETS rocblas-test COMPONENT tests)
rocm_install(FILES ${ROCBLAS_TEST_DATA} DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT tests)
