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

if(BUILD_STATIC_EXECUTABLE)
    target_link_libraries(SimpleMailClientCli Qt::Network)
    if(UNIX AND NOT APPLE)
        target_link_options(SimpleMailClientCli PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
