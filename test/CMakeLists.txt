set(SOURCE_CLNT ${SOURCE_STORAGE} ${CMAKE_CURRENT_SOURCE_DIR}/test/main.cc)
add_executable(astsclnt ${SOURCE_SQL} ${SOURCE_CLNT})
target_link_libraries(astsclnt mtesrl ${LIB_STORAGE})
