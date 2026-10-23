cmake_minimum_required(VERSION 3.16)

project (wui LANGUAGES CXX C)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(WUI_BUILD_EXAMPLES "Build the WUI examples" ON)

set(SANITIZERS_ENABLE OFF) # ON

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
	set(SANITIZERS_DEFAULT ${SANITIZERS_ENABLE})
else()
	set(SANITIZERS_DEFAULT OFF)
endif()

option(ENABLE_SANITIZERS "Enable sanitizers for memory and UB testing" ${SANITIZERS_DEFAULT})

add_library(wui_sanitizers INTERFACE)

if(ENABLE_SANITIZERS)
	message(STATUS "[Sanitizers] Activated.")
	if(MSVC)
		target_compile_options(wui_sanitizers INTERFACE /fsanitize=address /Zi)
		target_compile_definitions(wui_sanitizers INTERFACE _DISABLE_VECTOR_ANNOTATION _DISABLE_STRING_ANNOTATION)
		target_link_options(wui_sanitizers INTERFACE /INCREMENTAL:NO)
	else()
		set(SAN_FLAGS -fsanitize=address,undefined -g -fno-omit-frame-pointer)
		set(SAN_LINK_FLAGS ${SAN_FLAGS} -fno-sanitize-recover=undefined)
		target_compile_options(wui_sanitizers INTERFACE ${SAN_FLAGS})
		target_link_options(wui_sanitizers INTERFACE ${SAN_LINK_FLAGS})
	endif(MSVC)
	set(BUILD_SHARED_LIBS ON)
else()
	set(BUILD_SHARED_LIBS OFF)
endif()

if(MSVC)
	add_compile_options(
		$<$<CONFIG:Release>:/bigobj>
	)
else()
	add_compile_options(
#		$<$<CONFIG:Release>:-march=native>
#		$<$<CONFIG:Release>:-O3>
		$<$<CONFIG:Debug>:-fno-omit-frame-pointer>
		$<$<CONFIG:Debug>:-fno-optimize-sibling-calls>
	)
	add_compile_definitions(
		$<$<CONFIG:Debug>:_DEBUG>
	)

	if(NOT ENABLE_SANITIZERS)
		add_link_options(
			$<$<CONFIG:Release>:-static-libstdc++>
			$<$<CONFIG:Release>:-static-libgcc>
		)
	endif()
endif()

include_directories(include)
add_subdirectory (src)

if(WUI_BUILD_EXAMPLES)
	add_subdirectory(examples)
endif()
