vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

vcpkg_download_distfile(
    ARCHIVE
    URLS "https://github.com/basil00/WinDivert/releases/download/v2.2.2/WinDivert-2.2.2-A.zip"
    FILENAME "WinDivert-2.2.2-A.zip"
    SHA512 92eb2ef98ced175d44de1cdb7c52f2ebc534b6a997926baeb83bfe94cba9287b438f796aff11f6163918bcdbc25bcd4e3383715f139f690d207ce219f846a345
)

vcpkg_extract_source_archive(SOURCE_PATH ARCHIVE "${ARCHIVE}")

if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    set(ARCH_DIR "x64")
    set(SYS_FILE "WinDivert64.sys")
elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    set(ARCH_DIR "x86")
    set(SYS_FILE "WinDivert32.sys")
else()
    message(FATAL_ERROR "Unsupported architecture: ${VCPKG_TARGET_ARCHITECTURE}")
endif()

file(COPY "${SOURCE_PATH}/include/" DESTINATION "${CURRENT_PACKAGES_DIR}/include")
file(COPY "${SOURCE_PATH}/${ARCH_DIR}/WinDivert.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
file(COPY "${SOURCE_PATH}/${ARCH_DIR}/WinDivert.dll" "${SOURCE_PATH}/${ARCH_DIR}/${SYS_FILE}"
     DESTINATION "${CURRENT_PACKAGES_DIR}/bin")

if(NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
    file(COPY "${SOURCE_PATH}/${ARCH_DIR}/WinDivert.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
    file(COPY "${SOURCE_PATH}/${ARCH_DIR}/WinDivert.dll" "${SOURCE_PATH}/${ARCH_DIR}/${SYS_FILE}"
         DESTINATION "${CURRENT_PACKAGES_DIR}/debug/bin")
endif()

file(WRITE "${CURRENT_PACKAGES_DIR}/share/windivert/WinDivertConfig.cmake" [=[
get_filename_component(_windivert_root "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

if(NOT TARGET WinDivert::WinDivert)
    add_library(WinDivert::WinDivert SHARED IMPORTED)

    set_target_properties(WinDivert::WinDivert PROPERTIES
        IMPORTED_CONFIGURATIONS "RELEASE;DEBUG"
        IMPORTED_LOCATION_RELEASE "${_windivert_root}/bin/WinDivert.dll"
        IMPORTED_IMPLIB_RELEASE "${_windivert_root}/lib/WinDivert.lib"
        IMPORTED_LOCATION_DEBUG "${_windivert_root}/debug/bin/WinDivert.dll"
        IMPORTED_IMPLIB_DEBUG "${_windivert_root}/debug/lib/WinDivert.lib"
        INTERFACE_INCLUDE_DIRECTORIES "${_windivert_root}/include"
    )
endif()
]=])

file(WRITE "${CURRENT_PACKAGES_DIR}/share/windivert/usage" [=[
The package windivert provides CMake targets:

    find_package(WinDivert CONFIG REQUIRED)
    target_link_libraries(main PRIVATE WinDivert::WinDivert)

Note: WinDivert.dll requires the kernel driver (WinDivert64.sys on x64,
WinDivert32.sys on x86) to be located in the same directory at runtime.
The .sys file is installed to <triplet>/bin, but is NOT deployed by
vcpkg's app-local mechanism (which only handles .dll files) - copy it
next to your executable yourself.
]=])

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")
