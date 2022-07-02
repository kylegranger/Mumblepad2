include_directories(../lib/include)

add_executable(test 
    src/main.cpp
)

if(USE_OPENGL)
    target_link_libraries(test
        mumblepad
        pthread
        dl
        glfw
        GL
    )
else()
    target_link_libraries(test
        mumblepad
        pthread
    )
endif()