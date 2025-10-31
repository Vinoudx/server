aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LISTS)

add_executable(testlogger testlogger.cc ${SRC_LISTS})

add_executable(testfiber testfiber.cc ${SRC_LISTS})

add_executable(testrandom testrandom.cc ${SRC_LISTS})

add_executable(testinet testinet.cc ${SRC_LISTS})

add_executable(benchmark benchmark.cc ${SRC_LISTS})

add_executable(testserver testserver.cc ${SRC_LISTS})

add_executable(udptester udptester.cc ${SRC_LISTS})
