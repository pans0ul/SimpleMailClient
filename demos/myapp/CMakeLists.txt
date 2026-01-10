set(demo_SRCS
    myapp.cpp
)

add_executable(myapp
    ${demo_SRCS}
)

target_link_libraries(myapp
    SimpleMail::Core
    Qt::Core
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
