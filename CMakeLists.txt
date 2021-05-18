#环境变量说明:
#CMAKE_SOURCE_DIR  CMakeList.txt所在的目录
#CMAKE_SOURCE_DIR,PROJECT_SOURCE_DIR,_SOURCE_DIR： 这三个变量内容一致，都指的是工程的顶级目录。
#PROJECT_BINARY_DIR 指执行 cmake的路径

PROJECT(my_prj_name)

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

#cotire设置gcc的预编译头相关
SET (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
INCLUDE(cotire)

# include directories
INCLUDE_DIRECTORIES(system ./include/)
INCLUDE_DIRECTORIES(system ./external/libevent_cpp/External/libevent-2.1.8-stable/out/include)
INCLUDE_DIRECTORIES(after  ${PROJECT_SOURCE_DIR})
INCLUDE_DIRECTORIES(after  ./external)

#运行cmake输出终端消息，调试可以用
message(PROJECT_SOURCE_DIR_is: ${PROJECT_SOURCE_DIR})

# lib directories
LINK_DIRECTORIES(
  #项目生成的静态链接库
  ${CMAKE_SOURCE_DIR}/lib

  #项目引用第三方库
  ${PROJECT_SOURCE_DIR}/acc_proto/lib
  ${PROJECT_SOURCE_DIR}/acc_driver/lib
  ${PROJECT_SOURCE_DIR}/external/svr_util/lib
  ${PROJECT_SOURCE_DIR}/external/libevent_cpp/lib

  #系统目录
  /usr/local/lib
  /use/lib
)


#指定编译器
if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -Wall -lrt -pthread -DDEBUG -Werror -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -Wall -lrt -pthread   -Werror -DDEBUG -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
else( CMAKE_BUILD_TYPE STREQUAL "Release" )
    #set(CMAKE_BUILD_TYPE "Release")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O2 -lrt -DGMPERMISSION -pthread -Werror -Wno-unused-result -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free ")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -lrt -O2 -DGMPERMISSION -pthread -Werror -Wno-unused-result -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free")
endif( CMAKE_BUILD_TYPE STREQUAL "Debug" )

#set(CMAKE_C_COMPILER /usr/bin/gcc)
#set(CMAKE_CXX_COMPILER /usr/bin/g++)
#SET(CMAKE_CPP_FLAGS_DEBUG "-g -Wall -std=c++11 -Wno-format") 

SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)
#SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/lib)

ADD_SUBDIRECTORY(acc_proto)
ADD_SUBDIRECTORY(acc_svr)
ADD_SUBDIRECTORY(acc_driver)
ADD_SUBDIRECTORY(test)
ADD_SUBDIRECTORY(samples)

#自定义命令
##################################
#ADD_CUSTOM_COMMAND(
#	 WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
#     COMMENT "cp acc_svr/cfg Debug/bin/ -rf"
#           )
#
##################################
