cmake_minimum_required(VERSION 3.0)
project(astsclnt)


set(SOURCE_SQL ../src/asts_connection.cc ../src/asts_interface.cc)
set(SOURCE_STORAGE ../src/storage/sqlite.cc)
set(SOURCE_CLNT ${SOURCE_STORAGE} main.cc)
set(SOURCE_LIB ../src/python_proxy.cc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -pedantic) # -Werror

find_package(Python3 COMPONENTS Interpreter Development)
find_package(Boost COMPONENTS python3 REQUIRED)

include_directories(../src/ /opt/mtesrl/embedded ${Python3_INCLUDE_DIRS})

add_library(mtesrl SHARED IMPORTED)
set_property(TARGET mtesrl PROPERTY IMPORTED_LOCATION /opt/mtesrl/embedded/libmtesrl.so)

set(CMAKE_SHARED_MODULE_PREFIX "")
add_library(astslib MODULE ${SOURCE_SQL} ${SOURCE_LIB} ${SOURCE_STORAGE})
target_link_libraries(astslib mtesrl sqlite3 ${Boost_LIBRARIES} ${Python3_LIBRARIES})

add_executable(astsclnt ${SOURCE_SQL} ${SOURCE_CLNT})
target_link_libraries(astsclnt mtesrl sqlite3)

set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -g")
