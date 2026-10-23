file(GLOB SOURCES
	*.cpp
	MainFrame/*.cpp)

include_directories(.)

add_executable(hello_world ${SOURCES})

target_link_libraries(hello_world PRIVATE wui_sanitizers
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
	stdc++fs
	udev
)

add_custom_command(TARGET hello_world POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:hello_world>/res")
