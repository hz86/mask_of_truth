cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(mask_of_truth)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -std=gnu99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -fno-rtti -fno-exceptions")

###############################################

include_directories(mask_of_truth
  common
)

add_executable(mask_of_truth
  common/rbtree.c
  kernel/main.c
  kernel/myio.c
  kernel/map.c
)

target_link_libraries(mask_of_truth
  SceIofilemgrForDriver_stub 
  SceModulemgrForKernel_stub 
  SceModulemgrForDriver_stub 
  SceThreadmgrForDriver_stub
  SceSysclibForDriver_stub
  SceSysrootForKernel_stub
  SceSysmemForDriver_stub
  taihenForKernel_stub
  gcc
  k
)

set_target_properties(mask_of_truth
  PROPERTIES LINK_FLAGS "-nostdlib"
  COMPILE_FLAGS "-D__VITA_KERNEL__"
)

vita_create_self(mask_of_truth.skprx mask_of_truth CONFIG kernel/exports.yml UNSAFE)
vita_create_stubs(mask_of_truth_stubs mask_of_truth kernel/exports.yml KERNEL)

###############################################

include_directories(mask_of_truth_user
  common
)

link_directories(mask_of_truth_user
  mask_of_truth_stubs
)

add_executable(mask_of_truth_user
  common/rbtree.c
  common/sha1.c
  user/main.c
  user/map.c
)

target_link_libraries(mask_of_truth_user
  mask_of_truth_stub
  SceIofilemgr_stub 
  SceKernelModulemgr_stub
  SceKernelThreadMgr_stub
  SceLibKernel_stub
  SceSysmodule_stub
  SceDisplay_stub
  SceSysmem_stub
  SceLibc_stub
  taihen_stub
)

set_target_properties(mask_of_truth_user
  PROPERTIES LINK_FLAGS "-nostdlib"
)

add_dependencies(mask_of_truth_user 
  mask_of_truth_stubs
)

vita_create_self(mask_of_truth.suprx mask_of_truth_user CONFIG user/exports.yml UNSAFE)

