file(GLOB SOURCES
	*.cpp
	ButtonSheet/*.cpp
	InputSheet/*.cpp
	MainFrame/*.cpp
	MainSheet/*.cpp)

include_directories(.)

add_executable(demo ${SOURCES})

target_link_libraries(demo PRIVATE wui_sanitizers
	wui
	xcb
	xcb-cursor
	xcb-ewmh
	xcb-icccm
	xcb-image
	X11
	X11-xcb
	stdc++fs
	cairo
	pthread
	udev
)

add_custom_command(TARGET demo POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy_directory
		"${CMAKE_CURRENT_SOURCE_DIR}/res" "$<TARGET_FILE_DIR:demo>/res")
