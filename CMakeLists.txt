cmake_minimum_required(VERSION 3.23)

# set the project name
project(MySocket-Updater)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(EXPORT_COMPILE_COMMANDS ON)

# add the executable
add_executable(MySocket-Updater main.cpp json.hpp)

# set curl path
set(CURL_LIBRARY "E:/tools kit/curl/lib")
set(CURL_INCLUDE_DIR "E:/tools kit/curl/include")
set(CURL_ROOT_DIR "E:/tools kit/curl")
set(CURL_USE_STATIC_LIBS TRUE)
find_package(CURL REQUIRED)
set(CMAKE_AR gcc-ar)
set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-static-libstdc++ -static-libgcc")

target_compile_options(MySocket-Updater PRIVATE -fdiagnostics-color=always -pedantic -lcurl -O3)
target_link_options(MySocket-Updater PRIVATE -flto -s)

# add curl
target_include_directories(MySocket-Updater PRIVATE ${CURL_INCLUDE_DIR})
target_link_directories(MySocket-Updater PRIVATE ${CURL_LIBRARY})
target_link_libraries(MySocket-Updater PRIVATE curl)