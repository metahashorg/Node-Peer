include_directories(.)
include_directories(${SNIPER_INCLUDE_DIRS})

add_compile_definitions(GIT_SHA1="${GIT_SHA1}")
message("GIT Revision=${GIT_SHA1}")


set(PROJECT_LIBS
    ${CMAKE_THREAD_LIBS_INIT}
    ${BOOST_LIBRARIES}
    ${CONFIG++_LIBRARY}
    ssl.a crypto.a dl
    ${SECP256K1_LIBRARY}
    backtrace dl
    fmt::fmt
    stdc++fs.a
    tcmalloc
    -lcurl
    -lresolv
    )

set(PROJECT_SRC
    version.h
    error_code.h
    Config.h
    Config.cpp
    Proxy.h
    Proxy.cpp
    request/JsonRequest.h
    request/JsonRequest.cpp
    message.h
    message.cpp
    stats/Counters.h
    stats/Counters.cpp
    stats/Stats.h
    stats/Stats.cpp
    request/JsonTransaction.h
    request/JsonTransaction.cpp
    DomainGroup.cpp
    DomainGroup.h
    )

#APP Lib
add_library(${PROJECT_NAME}_lib STATIC ${PROJECT_SRC})

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}_lib ${SNIPER_LIBRARIES} ${PROJECT_LIBS})
