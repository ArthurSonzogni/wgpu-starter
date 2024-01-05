cmake_minimum_required (VERSION 3.11)
set(CMAKE_CXX_STANDARD 20)
set(FETCHCONTENT_QUIET OFF)
set(FETCHCONTENT_UPDATES_DISCONNECTED ON)
include(FetchContent)

project(example)
add_executable(example
  main.cpp
)

FetchContent_Declare(glm
  GIT_REPOSITORY https://github.com/g-truc/glm
	GIT_TAG b06b775c1c80af51a1183c0e167f9de3b2351a79
  GIT_PROGRESS TRUE
)

FetchContent_Declare(dawn
	DOWNLOAD_COMMAND
		cd ${FETCHCONTENT_BASE_DIR}/dawn-src &&
		git init &&
		git fetch --depth=1 https://dawn.googlesource.com/dawn chromium/6227 &&
		git reset --hard FETCH_HEAD
	GIT_PROGRESS TRUE
)

FetchContent_MakeAvailable(glm)
target_link_libraries(example PRIVATE glm)

if(EMSCRIPTEN)
  set_target_properties(example PROPERTIES SUFFIX ".html")
  target_link_options(example PRIVATE "-sUSE_WEBGPU=1")
	target_link_options(example PRIVATE "-sUSE_GLFW=3")
else()
	# -- Dawn ---------------------------------------------------------------------
	if (NOT dawn_POPULATED)
		set(DAWN_FETCH_DEPENDENCIES ON)

		# A more minimalistic choice of backend than Dawn's default
		if (APPLE)
			set(USE_VULKAN OFF)
			set(USE_METAL ON)
		else()
			set(USE_VULKAN ON)
			set(USE_METAL OFF)
		endif()
		set(DAWN_ENABLE_D3D11 OFF)
		set(DAWN_ENABLE_D3D12 OFF)
		set(DAWN_ENABLE_METAL ${USE_METAL})
		set(DAWN_ENABLE_NULL OFF)
		set(DAWN_ENABLE_DESKTOP_GL OFF)
		set(DAWN_ENABLE_OPENGLES OFF)
		set(DAWN_ENABLE_VULKAN ${USE_VULKAN})
		set(TINT_BUILD_SPV_READER OFF)

		# Disable unneeded parts
		set(DAWN_BUILD_SAMPLES OFF)
		set(TINT_BUILD_TINT OFF)
		set(TINT_BUILD_SAMPLES OFF)
		set(TINT_BUILD_DOCS OFF)
		set(TINT_BUILD_TESTS OFF)
		set(TINT_BUILD_FUZZERS OFF)
		set(TINT_BUILD_SPIRV_TOOLS_FUZZER OFF)
		set(TINT_BUILD_AST_FUZZER OFF)
		set(TINT_BUILD_REGEX_FUZZER OFF)
		set(TINT_BUILD_BENCHMARKS OFF)
		set(TINT_BUILD_TESTS OFF)
		set(TINT_BUILD_AS_OTHER_OS OFF)
		set(TINT_BUILD_REMOTE_COMPILE OFF)

		fetchcontent_populate(dawn)
		message(STATUS "Fetching Dawn")
		message(STATUS "  - dawn_source_dir: ${dawn_SOURCE_DIR}")
		message(STATUS "  - dawn_binary_dir: ${dawn_BINARY_DIR}")
		add_subdirectory(${dawn_SOURCE_DIR} ${dawn_BINARY_DIR} EXCLUDE_FROM_ALL)
	endif ()

	target_link_libraries(example
		PRIVATE glfw
		PRIVATE dawncpp
		PRIVATE webgpu_cpp
		PRIVATE webgpu_dawn
		PRIVATE webgpu_glfw
	)
endif()
