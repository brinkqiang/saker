cmake_minimum_required(VERSION 2.6)

project (saker C)

set (SAKER_VERSION_MAJOR 1)
set (SAKER_VERSION_MINOR 0)
set (SAKER_VERSION_PATCH 1)

option(ENABLE_DEBUG "Set to OFF|ON (default is OFF) to control Debug or Release" ON)
option(ENABLE_JEMALLOC "Set to OFF|ON (default is ON) to control use jemalloc or not" ON)
option(ENABLE_TESTS "Set to OFF|ON (default is OFF) to control build of tests " ON)

set(CMAKE_USE_RELATIVE_PATHS TRUE)

if (NOT CMAKE_BUILD_TYPE)
    if (ENABLE_DEBUG)
        set(CMAKE_BUILD_TYPE Debug)
    else ()
        set(CMAKE_BUILD_TYPE Release)  
    endif ()
endif ()

if (ENABLE_JEMALLOC)
    set(DepLibs 
        libjemalloc.a
        )
    set(CMAKE_C_FLAGS "-DUSE_JEMALLOC")
endif ()

if (UNIX)
    set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
    set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
    # -std=gnu99 -pedantic -Wshadow -Wunreachable-code -Wstrict-prototypes
    set(CMAKE_C_FLAGS "-Wall -Wstrict-prototypes -Wno-strict-aliasing -Wno-unused-function -Wno-strict-aliasing -Wunused-but-set-variable -DLUA_COMPAT_ALL")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g3 -DDEBUG -Werror")
    set(CMAKE_C_FLAGS_RELEASE "-O2 -g2 -DNDEBUG")
else ()
    message(FATAL_ERROR, "must use gcc in unix")
endif ()

set(LuaRocksDir ${EXECUTABLE_OUTPUT_PATH})
set(ThdInstallDir ${PROJECT_SOURCE_DIR}/cmake_build/thd-usr)
set(ThdDir ${PROJECT_SOURCE_DIR}/deps)
set(SqliteDir ${PROJECT_SOURCE_DIR}/deps/sqlite-3.8.8)

include_directories(${ThdInstallDir}/include)

link_directories(${PROJECT_SOURCE_DIR}/lib)
link_directories(${ThdInstallDir}/lib)

set(HiRedisLib ${ThdInstallDir}/lib/libhiredis.a)
set(LuaLib ${ThdInstallDir}/lib/liblua.a)
set(JeMallocLib ${ThdInstallDir}/lib/libjemalloc.a)

set(LuaSocketSO ${LuaLibDir}/unix.so)

add_custom_command(OUTPUT ${LuaLib}
                  PRE_BUILD 
                  COMMAND make posix MYCFLAGS="-DLUA_USE_DLOPEN -DLUA_COMPAT_ALL" MYLIBS="-ldl"
                  COMMAND make install INSTALL_TOP=${ThdInstallDir}
                  WORKING_DIRECTORY ${ThdDir}/lua-5.2.3
                  COMMENT "build lualib")

add_custom_command(OUTPUT ${LuaSocketSO}
                  PRE_BUILD 
                  COMMAND make PLAT=linux DEBUG=DEBUG LUAV=5.2 LUAINC_linux=${ThdInstallDir}/include prefix=${LuaRocksDir} linux
				  COMMAND make PLAT=linux DEBUG=DEBUG LUAV=5.2 LUAINC_linux=${ThdInstallDir}/include prefix=${LuaRocksDir} install
                  WORKING_DIRECTORY ${ThdDir}/luasocket/src
                  COMMENT "build luasocket")

add_custom_command(OUTPUT ${HiRedisLib}
                  PRE_BUILD 
                  COMMAND make static
                  COMMAND make PREFIX=${ThdInstallDir} install
                  WORKING_DIRECTORY ${ThdDir}/hiredis
                  COMMENT "build hiredis")

#set(JEMALLOC_CFLAGS "-std=gnu99 -Wall -pipe -g3 -O3 -funroll-loops" $(CFLAGS))
add_custom_command(OUTPUT ${JeMallocLib}
                  PRE_BUILD 
                  COMMAND chmod +x configure *sh
                  COMMAND chmod +x include/jemalloc/*.sh 
                  COMMAND chmod +x include/jemalloc/internal/*.sh 
                  COMMAND ./configure --prefix=${ThdInstallDir} --with-jemalloc-prefix=je_ --enable-cc-silence 
                  COMMAND make
                  COMMAND make install
                  WORKING_DIRECTORY ${ThdDir}/jemalloc-3.6.0
                  COMMENT "build libjemalloc")
                  
add_custom_target(run ALL DEPENDS ${LuaLib} ${HiRedisLib} ${JeMallocLib} ${LuaSocketSO})

if (ENABLE_TESTS)
    include(CTest)
    enable_testing()
    message(STATUS "Building with tests ")
else ()
    message(STATUS "Building without tests ")
endif ()

add_subdirectory(src)
add_subdirectory(src/client)

#make install
install(DIRECTORY ${EXECUTABLE_OUTPUT_PATH} DESTINATION saker
        PATTERN "*"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ
                    GROUP_EXECUTE GROUP_READ
                    WORLD_EXECUTE)

#cpack
include (InstallRequiredSystemLibraries)
set (CPACK_PACKAGE_VENDOR "cinience@hotmail.com")
set (CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/COPYRIGHT")
set (CPACK_PACKAGE_VERSION_MAJOR "${SAKER_VERSION_MAJOR}")
set (CPACK_PACKAGE_VERSION_MINOR "${SAKER_VERSION_MINOR}")
set (CPACK_PACKAGE_VERSION_PATCH "${SAKER_VERSION_PATCH}")
include (CPack)

message(STATUS "CMake ${CMAKE_VERSION} successfully configured ${PROJECT_NAME} using ${CMAKE_GENERATOR} generator")
message(STATUS "Installation target path: ${CMAKE_INSTALL_PREFIX}")
