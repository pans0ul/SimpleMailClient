set(SRC
    app_cli.cpp
)

add_executable(app_cli
    ${SRC}
)

target_link_libraries(app_cli
    SimpleMail::Core
    Qt::Core
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
