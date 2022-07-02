include_directories(include)

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