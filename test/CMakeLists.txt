include_directories(../lib/include)

add_executable(test 
    src/main.cpp
)

target_link_libraries(test
    mumblepad
    pthread
    dl
    glfw
    GL
)