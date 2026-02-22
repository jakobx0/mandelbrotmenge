cmake_minimum_required(VERSION 3.15)
project(sc LANGUAGES CXX)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
	message(FATAL_ERROR "In-tree builds are not supported. Run CMake from a separate directory: cmake -S . -B build/debug")
endif()

# Keep cmkr optional, but disable auto-generation by default so setup works offline.
set(CMKR_SKIP_GENERATION ON CACHE BOOL "Skip automatic cmkr generation")
set(CMKR_ROOT_PROJECT OFF)
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
	set(CMKR_ROOT_PROJECT ON)

	# Bootstrap cmkr and automatically regenerate CMakeLists.txt
	include(cmkr.cmake OPTIONAL RESULT_VARIABLE CMKR_INCLUDE_RESULT)
	if(CMKR_INCLUDE_RESULT)
		cmkr()
	endif()

	# Enable folder support
	set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# CMake 3.30+: prefer Boost config packages and suppress CMP0167 warning.
if(POLICY CMP0167)
	cmake_policy(SET CMP0167 NEW)
endif()

# Packages
find_package(Boost REQUIRED
	COMPONENTS
		thread
		chrono
)

find_package(SDL REQUIRED)

# Target: server
set(server_SOURCES
	"src/server/server.cpp"
	"src/server/udp_server.cpp"
	cmake.toml
)

add_executable(server)

target_sources(server PRIVATE ${server_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${server_SOURCES})

target_compile_features(server PRIVATE
	cxx_std_14
)

target_include_directories(server PRIVATE
	include
)

target_link_libraries(server PRIVATE
	Boost::thread
	Boost::chrono
	SDL::SDL
)

get_directory_property(CMKR_VS_STARTUP_PROJECT DIRECTORY ${PROJECT_SOURCE_DIR} DEFINITION VS_STARTUP_PROJECT)
if(NOT CMKR_VS_STARTUP_PROJECT)
	set_property(DIRECTORY ${PROJECT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT server)
endif()

# Target: client
set(client_SOURCES
	"src/client/client.cpp"
	"src/client/udp_client.cpp"
	cmake.toml
)

add_executable(client)

target_sources(client PRIVATE ${client_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} FILES ${client_SOURCES})

target_compile_features(client PRIVATE
	cxx_std_14
)

target_include_directories(client PRIVATE
	include
)

target_link_libraries(client PRIVATE
	Boost::thread
	Boost::chrono
)
