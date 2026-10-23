file(GLOB SOURCES
	common/*.cpp
	control/*.cpp
	config/*.cpp
	event/*.cpp
	framework/*.cpp
	graphic/*.cpp
	theme/*.cpp
	theme/impl/*.cpp
	locale/*.cpp
	locale/impl/*.cpp
	system/*.cpp
	window/*.cpp)

find_package(PkgConfig REQUIRED)
pkg_check_modules(CAIRO REQUIRED cairo)

include_directories(.
	../thirdparty
	PUBLIC ${CAIRO_INCLUDE_DIRS}
)

add_library(wui ${SOURCES})

target_link_libraries(wui PRIVATE wui_sanitizers)

target_include_directories(wui PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}
)

#message(STATUS "BUILD_SHARED_LIBS : ${BUILD_SHARED_LIBS}")
#message(STATUS "ENABLE_SANITIZERS : ${ENABLE_SANITIZERS}")
