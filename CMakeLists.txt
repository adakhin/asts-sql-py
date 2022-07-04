cmake_minimum_required(VERSION 3.0)
project(asts-sql-py)

set(SOURCE_SQL ${CMAKE_CURRENT_SOURCE_DIR}/src/asts_interface.cc)
set(SOURCE_LIB ${CMAKE_CURRENT_SOURCE_DIR}/src/python_proxy.cc)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
add_compile_options(-Wall -Wextra -pedantic) # -Werror

find_package(Python3 COMPONENTS Interpreter Development)
find_package(Boost COMPONENTS python3 REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/ /opt/mtesrl/embedded ${Python3_INCLUDE_DIRS})

add_library(mtesrl SHARED IMPORTED)
set_property(TARGET mtesrl PROPERTY IMPORTED_LOCATION /opt/mtesrl/embedded/libmtesrl.so)

# remove shared module prefix(xx.so instead of libxx.so)
set(CMAKE_SHARED_MODULE_PREFIX "")
set(CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} -g")

if(${ASTSSQLPY_SQLITE})
  include(${CMAKE_CURRENT_SOURCE_DIR}/src/storage/CMakeLists.sqlite.txt)
endif(${ASTSSQLPY_SQLITE})

if(${ASTSSQLPY_EXAMPLE})
  include(${CMAKE_CURRENT_SOURCE_DIR}/test/CMakeLists.txt)
endif(${ASTSSQLPY_EXAMPLE})

add_library(astslib MODULE ${SOURCE_SQL} ${SOURCE_LIB} ${SOURCE_STORAGE})
target_link_libraries(astslib mtesrl ${LIB_STORAGE} ${Boost_LIBRARIES} ${Python3_LIBRARIES})
