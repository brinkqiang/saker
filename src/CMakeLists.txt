
set(BinName ${PROJECT_NAME})

set(DepLibs 
     ${DepLibs}
     m
     dl
     pthread
     libhiredis.a
     liblua.a
    )

set(AeSrcFile 
    event/zmalloc.c
    event/adlist.c
    event/ae.c
    event/anet.c
    event/util.c
    event/sds.c
)

set(SqliteSrcFile
    ${SqliteDir}/sqlite3.c
    )
    
set(SubDir . utils core service sysinfo plugins proto luacompat)
list(APPEND BinSrcs ${AeSrcFile})
list(APPEND BinSrcs ${SqliteSrcFile})

foreach(TheDir ${SubDir})
    aux_source_directory(${TheDir} BinSrcs)
    if(NOT ${TheDir} STREQUAL ".")
        unset(TheSours)
        file(GLOB TheSours ${TheDir}/*.h)
        file(GLOB TheIncludes ${TheDir}/*.h)

        source_group(${TheDir} FILES ${TheSours} ${TheIncludes})
    endif() 
endforeach()

if (UNIX)
    aux_source_directory(sysinfo/linux BinSrcs)    
    file(GLOB SysInfoSours sysinfo/linux/*.[hc])
endif()

source_group(sysinfo FILES ${SysInfoSours})
include_directories(${PROJECT_SOURCE_DIR}/deps)
include_directories(${SqliteDir})
include_directories(${PROJECT_SOURCE_DIR}/src)
include_directories(${PROJECT_SOURCE_DIR}/src/event)

add_executable(${BinName} ${BinSrcs})

TARGET_LINK_LIBRARIES(${BinName}  ${DepLibs})
