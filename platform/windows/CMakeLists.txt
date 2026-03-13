if(NOT WIN32)
	return()
endif()

if(NOT TARGET appMosfer)
	return()
endif()

target_sources(appMosfer
	PRIVATE
		src/win_usb.cpp
)

target_include_directories(appMosfer
	PRIVATE
		${CMAKE_CURRENT_SOURCE_DIR}
)

if(EXISTS "${CMAKE_SOURCE_DIR}/common/raw_packet")
	target_include_directories(appMosfer PRIVATE ${CMAKE_SOURCE_DIR}/common/raw_packet)
endif()

target_compile_definitions(appMosfer
	PRIVATE
		WIN32_LEAN_AND_MEAN
		NOMINMAX
)

target_link_libraries(appMosfer
	PRIVATE
		setupapi
		cfgmgr32
)

if(TARGET raw_packet_lib)
	target_link_libraries(appMosfer PRIVATE raw_packet_lib)
endif()
