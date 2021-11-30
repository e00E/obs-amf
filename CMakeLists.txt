project(amftest)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
	if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
		string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	else()
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
	endif()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_EXTENSIONS False)

add_subdirectory(dependencies/fmt)

add_library(${PROJECT_NAME} MODULE
	source/amf.cpp
	source/amf.h
	source/encoder.cpp
	source/encoder.h
	source/encoder_details.h
	source/encoder_details_avc.cpp
	source/encoder_details_hevc.cpp
	source/gsl.h
	source/module.cpp
	source/module.h
	source/plugin.cpp
	source/settings.cpp
	source/settings.h
	source/texture_encoder.cpp
	source/texture_encoder.h
	source/util.cpp
	source/util.h
)

target_include_directories(${PROJECT_NAME}
	SYSTEM PRIVATE
		dependencies/include
		dependencies/fmt/include
)
target_link_libraries(${PROJECT_NAME}
	d3d11
	dxgi
	dxguid
	fmt::fmt
	libobs
)

install_obs_plugin(${PROJECT_NAME})
