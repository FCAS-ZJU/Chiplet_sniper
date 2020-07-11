project(interchiplet)
add_executable(interchiplet interchiplet.cpp)
find_package(cppzmq)
find_package(Boost REQUIRED COMPONENTS
    system
    filesystem
)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(interchiplet cppzmq)
target_link_libraries(interchiplet ${Boost_LIBRARIES})
find_package(Threads)
target_link_libraries (interchiplet ${CMAKE_THREAD_LIBS_INIT})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")

set(CMAKE_CXX_STANDARD 17)
