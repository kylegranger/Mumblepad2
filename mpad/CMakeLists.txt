include_directories(../lib/include)

add_executable(mpad 
    src/main.cpp
)

add_custom_command(TARGET mpad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mpad> ../../out/mpad
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


