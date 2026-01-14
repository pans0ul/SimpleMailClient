set(SRC
    main.cpp
)

add_executable(SimpleMailClientCli
    ${SRC}
)

target_link_libraries(SimpleMailClientCli
    SimpleMail::Core
    Qt::Core
)

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
