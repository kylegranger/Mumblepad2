include_directories(../lib/include)

add_executable(mpad 
    src/main.cpp
)

if(USE_OPENGL)
    target_link_libraries(mpad
        mumblepad
        pthread
        dl
        glfw
        GL
    )
else()
    target_link_libraries(mpad
        mumblepad
        pthread
    )
endif()