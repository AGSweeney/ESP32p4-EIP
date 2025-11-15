# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Unused components have been moved to components/unused/
# They are not included in the build to avoid unnecessary dependencies
# See components/unused/README.md for details

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)

# Define FD_SETSIZE for LWIP_MAX_SOCKETS=64
# LWIP_SOCKET_OFFSET = FD_SETSIZE - MAX_SOCKETS, and we need LWIP_SOCKET_OFFSET >= 6
# So FD_SETSIZE >= MAX_SOCKETS + 6 = 64 + 6 = 70
# Also need room for stdout, stderr, stdin (+3), so use 73 for safety
add_compile_definitions(FD_SETSIZE=73)

project(ESP32-P4-OpENerEIP)

# Copy firmware binary to FirmwareImages folder with unique name after build
# The copy script will generate a filename with timestamp and optional git commit hash
idf_build_get_property(binary_dir BINARY_DIR)
idf_build_get_property(project_name PROJECT_NAME)
set(source_binary "${binary_dir}/${project_name}.bin")
set(firmware_images_dir "${CMAKE_SOURCE_DIR}/FirmwareImages")
set(copy_script "${CMAKE_SOURCE_DIR}/scripts/copy_firmware.py")

# Find Python (ESP-IDF requires Python, so it should be available)
find_package(Python3 QUIET COMPONENTS Interpreter)
if(NOT Python3_Interpreter_FOUND)
    # Fallback to python command
    set(PYTHON_CMD "python")
else()
    set(PYTHON_CMD "${Python3_EXECUTABLE}")
endif()

# Create a custom command that runs after the binary is generated
# The binary is created by ESP-IDF's build system after linking
# The Python script includes retry logic to wait for the binary to be created
# We use a marker file to track when the copy has been done (for incremental builds)
set(copy_marker "${binary_dir}/.firmware_copied")
add_custom_command(
    OUTPUT "${copy_marker}"
    COMMAND ${CMAKE_COMMAND} -E env "PYTHONIOENCODING=utf-8"
        ${PYTHON_CMD} "${copy_script}" "${source_binary}" "${firmware_images_dir}" "${project_name}"
    COMMAND ${CMAKE_COMMAND} -E touch "${copy_marker}"
    # Note: We don't use DEPENDS on the binary file because CMake may not track it as a build artifact
    # The Python script includes retry logic to wait for the binary to be created
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMENT "Copying firmware to FirmwareImages with unique name"
    VERBATIM
)

# Create a custom target that depends on the marker file
# This ensures the copy command runs during the build
# We make it depend on the main project target to ensure it runs after linking
add_custom_target(copy_firmware_image ALL
    DEPENDS "${copy_marker}" ${project_name}.elf
)
