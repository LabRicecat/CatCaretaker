cmake_minimum_required(VERSION 3.9)

project(catcare 
    VERSION 1.0.0
    LANGUAGES CXX
    DESCRIPTION "A tool to easily reuse projects"
)

set(CMAKE_CXX_STANDARD 20)
set(BINARY catcare)
set(SOURCE 
    src/main.cpp 
    src/configs.cpp 
    src/network.cpp 
    src/options.cpp 
    src/pagelist.cpp 

    mods/ArgParser/ArgParser.cpp 
    )

add_executable(${BINARY} ${SOURCE})

if(UNIX)
target_link_libraries(${BINARY} curl)
target_compile_options(${BINARY} PUBLIC -g)
endif()

install(TARGETS ${BINARY})