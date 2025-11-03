aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LISTS)

add_executable(testlogger testlogger.cc ${SRC_LISTS})
target_link_libraries(testlogger PRIVATE pthread)

add_executable(testfiber testfiber.cc ${SRC_LISTS})
target_link_libraries(testfiber PRIVATE pthread)

add_executable(testrandom testrandom.cc ${SRC_LISTS})
target_link_libraries(testrandom PRIVATE pthread)

add_executable(testinet testinet.cc ${SRC_LISTS})
target_link_libraries(testinet PRIVATE pthread)

add_executable(benchmark benchmark.cc ${SRC_LISTS})
target_link_libraries(benchmark PRIVATE pthread)

add_executable(testserver testserver.cc ${SRC_LISTS})
target_link_libraries(testserver PRIVATE pthread)

add_executable(udptester udptester.cc ${SRC_LISTS})
target_link_libraries(udptester PRIVATE pthread)

add_executable(testudpbroadcast testudpbroadcast.cc ${SRC_LISTS})
target_link_libraries(testudpbroadcast PRIVATE pthread)

add_executable(testkcp testkcp.cc ${SRC_LISTS})
target_link_libraries(testkcp PRIVATE pthread)
