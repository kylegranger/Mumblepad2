include_directories(include)

if (USE_OPENGL)
    add_library(mumblepad
        src/mumblepad.cpp
        src/mumblepadmt.cpp
        src/mumblepadthread.cpp
        src/mumblepadgla.cpp
        src/mumblepadglb.cpp
        src/mumprng.cpp
        src/mumengine.cpp
        src/mumpublic.cpp
        src/mumrenderer.cpp
        src/mumglwrapper.cpp
        src/signal.cpp
        src/signal.cpp
        gl/gl3w.c
    )
else()
    add_library(mumblepad
        src/mumblepad.cpp
        src/mumblepadmt.cpp
        src/mumblepadthread.cpp
        src/mumprng.cpp
        src/mumengine.cpp
        src/mumpublic.cpp
        src/mumrenderer.cpp
        src/signal.cpp
        src/signal.cpp
    )
endif()

add_custom_command(TARGET mumblepad POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:mumblepad> ../../out/libmumblepad.a
)


