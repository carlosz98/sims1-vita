cmake_minimum_required(VERSION 3.16)

# Vita builds: cmake -DCMAKE_TOOLCHAIN_FILE=$VITASDK/share/vita.toolchain.cmake ..
if(DEFINED ENV{VITASDK} AND CMAKE_TOOLCHAIN_FILE MATCHES "vita")
  set(PB_VITA ON)
endif()

project(PlumbBobVita CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(pbcore STATIC
  src/formats/binary_reader.cpp
  src/formats/far.cpp
  src/formats/iff.cpp
  src/formats/bhav.cpp
  src/vm/vm.cpp
)
target_include_directories(pbcore PUBLIC src)

if(NOT PB_VITA)
  if(NOT MSVC)
    target_compile_options(pbcore PRIVATE -Wall -Wextra)
  endif()

  add_executable(simtool tools/simtool.cpp)
  target_link_libraries(simtool pbcore)

  enable_testing()
  add_executable(pbtests tests/tests.cpp)
  target_link_libraries(pbtests pbcore)
  add_test(NAME pbtests COMMAND pbtests)
else()
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
  set(VITA_APP_NAME "PlumbBob Vita")
  set(VITA_TITLEID  "PLMB00001")
  set(VITA_VERSION  "00.01")

  add_executable(plumbbob src/vita/main.cpp)
  target_link_libraries(plumbbob pbcore)
  vita_create_self(eboot.bin plumbbob)
  vita_create_vpk(PlumbBobVita.vpk ${VITA_TITLEID} eboot.bin
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
  )
endif()
