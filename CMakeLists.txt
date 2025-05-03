cmake_minimum_required(VERSION 3.10)
project(RasterEditor)

set(CMAKE_CXX_STANDARD 17)

# include
include_directories(${CMAKE_SOURCE_DIR}/include)

add_executable(rastereditor
	src/main.cpp
	src/glad.c
	src/imgui.cpp
	src/imgui_draw.cpp
	src/imgui_impl_glfw.cpp
	src/imgui_impl_opengl3.cpp
	src/imgui_tables.cpp
	src/imgui_widgets.cpp
)
target_include_directories(rastereditor PRIVATE src)

target_link_libraries(rastereditor
	${CMAKE_SOURCE_DIR}/lib/libglfw3.a
)
