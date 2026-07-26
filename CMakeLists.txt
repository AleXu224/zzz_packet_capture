cmake_minimum_required(VERSION 3.21)
set(VCPKG_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/glt/vcpkg")
set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
set(VCPKG_OVERLAY_PORTS "${CMAKE_CURRENT_SOURCE_DIR}/vcpkg-overlay-ports")
set(X_VCPKG_APPLOCAL_DEPS_INSTALL ON)
project(zzz_packet_capture VERSION 0.1.0 LANGUAGES C CXX)

add_custom_target(Assets)
# Create the assets folder
add_custom_command(TARGET Assets PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_FILE_DIR:zzz_packet_capture>/assets)
# Make a copy in the executable directory
add_custom_command(TARGET Assets PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" $<TARGET_FILE_DIR:zzz_packet_capture>/assets)
# Make an additional copy in the binary dir in case a multi config generator is used
add_custom_command(TARGET Assets PRE_BUILD
COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/assets" ${PROJECT_BINARY_DIR}/assets)

file(GLOB_RECURSE all_files CONFIGURE_DEPENDS "src/*.cpp" "src/*.cc" "src/*.h" "src/*.hpp")
add_executable(zzz_packet_capture main.cpp ${all_files})
target_compile_features(zzz_packet_capture PUBLIC cxx_std_23)
target_include_directories(zzz_packet_capture PRIVATE src)
target_compile_definitions(zzz_packet_capture PRIVATE NOMINMAX)

# Require admin privileges so that WinDivert driver can be loaded
target_link_options(zzz_packet_capture PRIVATE
    "LINKER:/manifest:embed"
)

target_link_options(zzz_packet_capture PRIVATE
	"$<$<NOT:$<CONFIG:Debug>>:LINKER:/manifestuac:level='requireAdministrator' uiAccess='false'>"
    "$<$<NOT:$<CONFIG:Debug>>:LINKER:/SUBSYSTEM:WINDOWS>"
    "$<$<NOT:$<CONFIG:Debug>>:LINKER:/ENTRY:mainCRTStartup>"
)
add_dependencies(zzz_packet_capture Assets)

add_subdirectory(glt)

target_link_libraries(zzz_packet_capture PRIVATE glt)

find_package(WinDivert CONFIG REQUIRED)

add_custom_command(TARGET zzz_packet_capture POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin/WinDivert64.sys"
        "$<TARGET_FILE_DIR:zzz_packet_capture>/WinDivert64.sys"
)

find_package(PcapPlusPlus CONFIG REQUIRED)
target_link_libraries(zzz_packet_capture PRIVATE PcapPlusPlus::Pcap++ PcapPlusPlus::Common++)

find_package(glaze CONFIG REQUIRED)
target_link_libraries(zzz_packet_capture PRIVATE glaze::glaze)

find_package(OpenSSL REQUIRED)
target_link_libraries(zzz_packet_capture PRIVATE OpenSSL::SSL OpenSSL::Crypto)

find_package(protobuf CONFIG REQUIRED)
target_link_libraries(zzz_packet_capture PRIVATE protobuf::libupb protobuf::libprotoc protobuf::libprotobuf protobuf::libprotobuf-lite)

get_property(_imported_targets DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY IMPORTED_TARGETS)
foreach(_t IN LISTS _imported_targets)
	get_target_property(_libs ${_t} INTERFACE_LINK_LIBRARIES)
	if(_libs)
		list(FILTER _libs EXCLUDE REGEX "^-ignore:[0-9]+$")
		set_property(TARGET ${_t} PROPERTY INTERFACE_LINK_LIBRARIES "${_libs}")
	endif()
	get_target_property(_opts ${_t} INTERFACE_LINK_OPTIONS)
	if(_opts)
		list(FILTER _opts EXCLUDE REGEX "^-ignore:[0-9]+$")
		set_property(TARGET ${_t} PROPERTY INTERFACE_LINK_OPTIONS "${_opts}")
	endif()
endforeach()

set_target_properties(glaze::glaze PROPERTIES
    INTERFACE_COMPILE_OPTIONS "\$<\$<COMPILE_LANG_AND_ID:CXX,MSVC>:/Zc:preprocessor;/permissive-;/Zc:lambda;\$<\$<CONFIG:Release>:/GL>;\$<\$<CONFIG:MinSizeRel>:/GL>>"
    INTERFACE_LINK_OPTIONS "\$<\$<COMPILE_LANG_AND_ID:CXX,MSVC>:\$<\$<CONFIG:Release>:/LTCG;/INCREMENTAL:NO>;\$<\$<CONFIG:MinSizeRel>:/LTCG;/INCREMENTAL:NO>>"
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
