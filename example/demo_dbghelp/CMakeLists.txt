#
# Set up the source files and output library
#
set(CURRENT_PROJECT demo_dbghelp)
set(SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

file(
	GLOB HEADER_FILES
	LIST_DIRECTORIES FALSE
	CONFIGURE_DEPENDS
	"${SOURCE_DIR}/*.h"
)

file(
	GLOB SOURCE_FILES
	LIST_DIRECTORIES FALSE
	CONFIGURE_DEPENDS
	"${SOURCE_DIR}/*.cpp"
)

source_group(
	TREE "${SOURCE_DIR}/.."
	FILES
		${HEADER_FILES}
		${SOURCE_FILES}
)

add_library(
	${CURRENT_PROJECT}
	SHARED
		${HEADER_FILES}
		${SOURCE_FILES}
)

target_precompile_headers(
	${CURRENT_PROJECT}
	PRIVATE
		pch.h
)

target_include_directories(
	${CURRENT_PROJECT}
	PRIVATE
		"${SOURCE_DIR}"
		"${SOURCE_DIR}/../../include/"
)

#
# Compiler-specific options
#
target_compile_features(
	${CURRENT_PROJECT}
	PRIVATE
		cxx_std_23
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	target_compile_options(
		${CURRENT_PROJECT}
		PRIVATE
			"/utf-8"
			"/sdl"
			"/permissive-"
			"/Zc:preprocessor"
			"/Zc:inline"
			"/EHsc"

			"/W4"
			"/wd4100"	# '': unreferenced formal parameter
			"/wd4200"	# nonstandard extension used: zero-sized array in struct/union
			"/wd4201"	# nonstandard extension used: nameless struct/union
			"/wd4324"	# '': structure was padded due to alignment specifier
	)
endif()

target_compile_definitions(
	${CURRENT_PROJECT}
	PRIVATE
		NOMINMAX
		VC_EXTRALEAN
		WIN32_LEAN_AND_MEAN
)
