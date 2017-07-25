cmake_minimum_required(VERSION 2.8)

project(libbpws C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_GENERATED_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/generated)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_MACOSX_RPATH ON)

set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}")
if (NOT CPACK_GENERATOR)
  if (UNIX)
    set(CPACK_GENERATOR "TGZ")
  else ()
    set(CPACK_GENERATOR "ZIP")
  endif ()
endif ()
if (NOT CPACK_PACKAGE_FILE_NAME)
  set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}")
endif ()


if (MSVC)
  if (NOT MSVC_VERSION LESS 1500)
    set(ADDITIONAL_COMPILER_FLAGS "${ADDITIONAL_COMPILER_FLAGS} /MP")
    set(ADDITIONAL_JSON-C_C_FLAGS "/DSTRERROR_OVERRIDE_IMPL")
  endif ()
else ()
  # PIC for compatibility
  set(ADDITIONAL_JSON-C_C_FLAGS "-fPIC")
  set(ADDITIONAL_LIBWEBSOCKETS_C_FLAGS "-fPIC")
  set(ADDITIONAL_COMPILER_FLAGS "-fPIC")

  if (BPWS_GENERATE_COVERAGE)
    set(ADDITIONAL_FLAGS_DEBUG "-fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${ADDITIONAL_FLAGS_DEBUG}")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${ADDITIONAL_FLAGS_DEBUG}")
    if (CMAKE_COMPILER_IS_GNUCC AND CMAKE_COMPILER_IS_GNUCXX)
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -lgcov")
    else ()
      set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} --coverage")
    endif ()
  endif()
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ADDITIONAL_COMPILER_FLAGS}")


option(BPWS_BUILD_CHECK "Build Check as ExternalProject" OFF)
option(BPWS_BUILD_LIBWEBSOCKETS "Build LibWebSockets as ExternalProject" OFF)


include(ExternalProject)

if (BPWS_BUILD_LIBWEBSOCKETS)
  ExternalProject_Add(
    libwebsockets
    GIT_REPOSITORY git://github.com/warmcat/libwebsockets
    GIT_TAG "v2.2-stable"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}" "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}" "-DCMAKE_C_FLAGS:STRING=${ADDITIONAL_LIBWEBSOCKETS_C_FLAGS}" "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}" "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>" "-DLWS_WITHOUT_EXTENSIONS=ON" "-DLWS_WITHOUT_TESTAPPS=ON" "-DLWS_WITH_SSL:BOOL=ON"
  )

  set(LIBWEBSOCKETS_TARGET_NAME websockets${CMAKE_STATIC_LIBRARY_SUFFIX})

  add_library(websockets STATIC IMPORTED)
  add_dependencies(websockets libwebsockets)
  ExternalProject_Get_Property(libwebsockets INSTALL_DIR)
  set_property(TARGET websockets PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${LIBWEBSOCKETS_TARGET_NAME})

  set(LIBWEBSOCKETS_INCLUDE_DIRS ${INSTALL_DIR}/include)
  set(LIBWEBSOCKETS_LIBRARIES websockets)

  find_package(OpenSSL REQUIRED)
  list(APPEND LIBWEBSOCKETS_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR})
  list(APPEND LIBWEBSOCKETS_LIBRARIES ${OPENSSL_LIBRARIES})

  if (NOT MSVC)
    list(APPEND LIBWEBSOCKETS_LIBRARIES z)
  endif ()
else ()
  find_package(LibWebSockets)
  if (NOT LIBWEBSOCKETS_FOUND)
    message(FATAL_ERROR "Call cmake with '-DBPWS_BUILD_LIBWEBSOCKETS=ON' to build LibWebSockets as ExternalProject.")
  endif ()
endif ()

if (BPWS_BUILD_JSONC)
  ExternalProject_Add(
    json-c
    GIT_REPOSITORY https://github.com/json-c/json-c
    GIT_TAG "master"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}" "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}" "-DCMAKE_C_FLAGS:STRING=${ADDITIONAL_JSON-C_C_FLAGS}" "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}" "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
  )
  add_library(json-c-static STATIC IMPORTED)
  add_dependencies(json-c-static json-c)
  ExternalProject_Get_Property(json-c INSTALL_DIR)
  set_property(TARGET json-c-static PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/json-c.lib)

  set(JSON-C_INCLUDE_DIRS ${INSTALL_DIR}/include)
  set(JSON-C_LIBRARIES json-c-static)
else ()
  find_package(json-c)
  if (NOT JSON-C_FOUND)
    message(FATAL_ERROR "Call cmake with '-DBPWS_BUILD_JSONC=ON' to build json-c as ExternalProject.")
  endif ()
endif ()

if (BPWS_BUILD_CHECK)
  ExternalProject_Add(
    check-code
    GIT_REPOSITORY git://github.com/libcheck/check
    GIT_TAG "0.11.0"
    CMAKE_ARGS "-DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}" "-DCMAKE_TOOLCHAIN_FILE:PATH=${CMAKE_TOOLCHAIN_FILE}" "-DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}" "-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>"
    BUILD_IN_SOURCE 1
  )

  add_library(check STATIC IMPORTED)
  add_library(compat STATIC IMPORTED)
  add_dependencies(check check-code)
  ExternalProject_Get_Property(check-code INSTALL_DIR)
  ExternalProject_Get_Property(check-code SOURCE_DIR)
  set_property(TARGET check PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}check${CMAKE_STATIC_LIBRARY_SUFFIX})
  set_property(TARGET compat PROPERTY IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}compat${CMAKE_STATIC_LIBRARY_SUFFIX})

  set(CHECK_INCLUDE_DIRS ${INSTALL_DIR}/include ${SOURCE_DIR})
  set(CHECK_LIBRARIES check compat)
  set(CHECK_FOUND TRUE)
else ()
  find_package(Check)
  find_package(Subunit)
  if (NOT CHECK_FOUND)
    message(WARNING "Call cmake with '-DBPWS_BUILD_CHECK=ON' to build Check as ExternalProject. Not performing tests.")
  endif ()
  if (SUBUNIT_FOUND)
    list(APPEND CHECK_LIBRARIES ${SUBUNIT_LIBRARIES})
  endif ()
endif ()

if (UNIX AND NOT APPLE)
  list(APPEND CHECK_LIBRARIES m pthread rt)
endif ()

include_directories(${LIBWEBSOCKETS_INCLUDE_DIRS} ${JSON-C_INCLUDE_DIRS} include src)

set(BPWS_PUBLIC_HEADERS
  include/bpws.h
)

set(BPWS_PRIVATE_HEADERS
  src/bpws_cbor.h
)

set(BPWS_SOURCES
  src/bpws.c
)

set(BPWS_LWS_PUBLIC_HEADERS
  include/bpws_lws.h
  include/bpws_lws_main.h
)

set(BPWS_LWS_SOURCES
  src/bpws_lws.c
  src/bpws_lws_main.c
)

set(BPWS_TEST_FILES
  test/test.h
  test/test.c
)

set(EXAMPLE_SOURCES
  example/example.c
)

set(BPWS_LIBRARIES "")

if (WINCE)
  list(APPEND BPWS_LIBRARIES ws2)
elseif (WIN32)
  list(APPEND BPWS_LIBRARIES ws2_32)
endif ()

source_group("Headers Private" FILES ${BPWS_PRIVATE_HEADERS})
source_group("Headers Public" FILES ${BPWS_PUBLIC_HEADERS} ${BPWS_LWS_PUBLIC_HEADERS})
source_group("Sources" FILES ${BPWS_SOURCES} ${BPWS_LWS_SOURCES})
source_group("Tests" FILES ${BPWS_TEST_FILES})

add_library(bpws SHARED ${BPWS_PUBLIC_HEADERS} ${BPWS_PRIVATE_HEADERS} ${BPWS_SOURCES})
set_property(TARGET bpws PROPERTY COMPILE_DEFINITIONS BPWS_EXPORTS)
set_property(TARGET bpws PROPERTY PUBLIC_HEADER ${BPWS_PUBLIC_HEADERS})
target_link_libraries(bpws ${BPWS_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES} ${JSON-C_LIBRARIES})
install(TARGETS bpws
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

add_library(bpws-lws SHARED ${BPWS_LWS_PUBLIC_HEADERS} ${BPWS_LWS_SOURCES})
set_property(TARGET bpws-lws PROPERTY COMPILE_DEFINITIONS BPWS_LWS_EXPORTS)
set_property(TARGET bpws-lws PROPERTY PUBLIC_HEADER ${BPWS_LWS_PUBLIC_HEADERS})
target_link_libraries(bpws-lws bpws ${LIBWEBSOCKETS_LIBRARIES} ${JSON-C_LIBRARIES})
if (NOT WIN32)
  target_link_libraries(bpws-lws pthread)
endif ()
install(TARGETS bpws-lws
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  PUBLIC_HEADER DESTINATION include
)

add_executable(example ${EXAMPLE_SOURCES})
target_link_libraries(example bpws-lws)

if (CHECK_FOUND)
  include_directories(${CHECK_INCLUDE_DIRS})
  add_executable(test_bpws ${BPWS_TEST_FILES} ${BPWS_PUBLIC_HEADERS} ${BPWS_PRIVATE_HEADERS} ${BPWS_SOURCES})
  set_property(TARGET test_bpws PROPERTY COMPILE_DEFINITIONS BPWS_STATIC BPWS_LWS_STATIC BPWS_USE_CUSTOM_ALLOCATOR)
  target_link_libraries(test_bpws ${CHECK_LIBRARIES} ${BPWS_LIBRARIES} ${LIBWEBSOCKETS_LIBRARIES} ${JSON-C_LIBRARIES})
	
  enable_testing()
  add_test(NAME bpws COMMAND test_bpws)
endif ()


find_program(GENHTML_EXECUTABLE NAMES genhtml)
find_program(LCOV_EXECUTABLE NAMES lcov)
if (GENHTML_EXECUTABLE AND LCOV_EXECUTABLE)
  add_custom_target(coverage_reset
    COMMAND ${LCOV_EXECUTABLE} --directory ${CMAKE_BINARY_DIR} --zerocounters
    COMMAND ${LCOV_EXECUTABLE} --directory ${CMAKE_BINARY_DIR} --capture --initial --output-file ${CMAKE_BINARY_DIR}/coverage_base.info
  )
  add_custom_target(coverage_generate
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/coverage
    COMMAND ${LCOV_EXECUTABLE} --directory ${CMAKE_BINARY_DIR} --capture --output-file ${CMAKE_BINARY_DIR}/coverage_test.info
    COMMAND lcov --extract ${CMAKE_BINARY_DIR}/coverage_test.info "${CMAKE_SOURCE_DIR}/src/*" --output-file ${CMAKE_BINARY_DIR}/coverage_test.info
    COMMAND ${GENHTML_EXECUTABLE} --baseline-file ${CMAKE_BINARY_DIR}/coverage_base.info --title ${CMAKE_PROJECT_NAME} --legend --show-details --num-spaces 4 --output-directory ${CMAKE_BINARY_DIR}/coverage ${CMAKE_BINARY_DIR}/coverage_test.info
  )
else ()
  message(STATUS "Can't generate coverage report: genhtml and/or lcov was not found")
endif ()

find_program(CPPCHECK_EXECUTABLE NAMES cppcheck)
find_program(CPPCHECKHTMLREPORT_EXECUTABLE NAMES cppcheck-htmlreport)
if (CPPCHECK_EXECUTABLE AND CPPCHECKHTMLREPORT_EXECUTABLE)
  add_custom_target(cppcheck
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/cppcheck
    COMMAND ${CPPCHECK_EXECUTABLE} --enable=style --force -j 8 --verbose --xml ${CMAKE_SOURCE_DIR} 3>&1 1>&2 2>&3 | ${CPPCHECKHTMLREPORT_EXECUTABLE} --title=${CMAKE_PROJECT_NAME} --report-dir=${CMAKE_BINARY_DIR}/cppcheck
  )
else ()
  message(STATUS "Can't run cppchek: cppcheck and/or cppcheck-htmlreport was not found")
endif ()

include(CPack)
