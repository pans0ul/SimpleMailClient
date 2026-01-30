find_package(Qt${QT_VERSION_MAJOR} 5.5.0 COMPONENTS Widgets REQUIRED)

set(demo_SRCS
    sendemail.cpp
    sendemailwidget.cpp
    main.cpp
)



add_executable(SimpleMailClient
    ${demo_SRCS}
)

target_link_libraries(SimpleMailClient
    SimpleMail::Core
    Qt::Core
    Qt::Widgets
)

target_link_options(SimpleMailClient PRIVATE
    "SHELL:-Wl,-rpath,$ORIGIN"
)

if(BUILD_STATIC_EXECUTABLE)
    target_link_libraries(SimpleMailClient Qt::Network)
    if(UNIX AND NOT APPLE)
        target_link_options(SimpleMailClient PRIVATE -static-libgcc -static-libstdc++)
    endif()
endif()


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)
