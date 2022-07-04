include_directories(../lib/include)

add_executable(test 
    src/main.cpp
)

add_custom_command(TARGET test POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:test> ../../out/test
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

