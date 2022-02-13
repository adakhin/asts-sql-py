cmake_minimum_required(VERSION 3.0)
project(astsclnt)


set(SOURCE_SQL ../src/asts_connection.cc ../src/asts_interface.cc)
set(SOURCE_CLNT main.cc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -pedantic) # -Werror

include_directories(../src/ /opt/mtesrl/embedded)

add_library(mtesrl SHARED IMPORTED)
set_property(TARGET mtesrl PROPERTY IMPORTED_LOCATION /opt/mtesrl/embedded/libmtesrl.so)

add_executable(astsclnt ${SOURCE_SQL} ${SOURCE_CLNT})
target_link_libraries(astsclnt mtesrl)

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -g")

target_include_directories(astsclnt
          INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}
          PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
          )
