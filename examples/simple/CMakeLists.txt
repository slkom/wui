file(GLOB SOURCES
	*.cpp)

include_directories(.)

add_executable(simple ${SOURCES})

target_link_libraries(simple PRIVATE wui_sanitizers
	wui
	xcb
	xcb-cursor
	xcb-ewmh
	xcb-icccm
	xcb-image
	X11
	X11-xcb
	cairo
	pthread
	udev
)

add_custom_command(TARGET simple POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:simple>/res")
