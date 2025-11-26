cmake_minimum_required(VERSION 3.16)
project(SiatkaGlutenowa)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native")

# GLAD
set(VENDOR_DIR "${CMAKE_SOURCE_DIR}/vendor")

include_directories(${VENDOR_DIR}/glad/include)
set(GLAD_SOURCE "${VENDOR_DIR}/glad/src/glad.c")
include_directories(${VENDOR_DIR}/imgui)
include_directories(${VENDOR_DIR}/imgui/backends)
file(GLOB IMGUI_SOURCES "${VENDOR_DIR}/imgui/*.cpp" "${VENDOR_DIR}/imgui/backends/imgui_impl_glfw.cpp" "${VENDOR_DIR}/imgui/backends/imgui_impl_opengl3.cpp")
include_directories(${VENDOR_DIR}/implot)
file(GLOB IMPLOT_SOURCES "${VENDOR_DIR}/implot/*.cpp")
include_directories(${VENDOR_DIR}/glm)
include_directories("${CMAKE_SOURCE_DIR}/src")

if(WIN32)
    include_directories(${VENDOR_DIR}/glfw/include)
    link_directories(${VENDOR_DIR}/glfw/lib-vc2022) 
else()
    find_package(glfw3 REQUIRED)
endif()

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_executable(${PROJECT_NAME} ${SOURCES} ${GLAD_SOURCE} ${IMGUI_SOURCES} ${IMPLOT_SOURCES})

if(WIN32)
    target_link_libraries(${PROJECT_NAME} glfw3 opengl32 gdi32 user32 shell32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_if_different "${VENDOR_DIR}/glfw/lib-vc2022/glfw3.dll" $<TARGET_FILE_DIR:${PROJECT_NAME}>)
else()
    target_link_libraries(${PROJECT_NAME} glfw GL dl)
endif()

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} OpenMP::OpenMP_CXX)
endif()
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/res")