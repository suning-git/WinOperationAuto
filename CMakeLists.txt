cmake_minimum_required(VERSION 3.20)
project(WinOpAutoMouseKeybdtest)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Force x64 architecture
set(CMAKE_GENERATOR_PLATFORM x64)

# Set compiler flags for MSVC
if(MSVC)
    add_compile_options(/W4)
    # Enable /std:c++20 explicitly
    add_compile_options(/std:c++20)
endif()

# Create executable
add_executable(WinOpAutoMouseKeybdtest
    src/main.cpp
    src/special_keys.cpp
    src/input_injection.cpp
    src/event_logger.cpp
    src/suggestion_overlay.cpp
)

# Link required Windows libraries
target_link_libraries(WinOpAutoMouseKeybdtest
    user32.lib
    gdi32.lib
)

# Set the manifest file - disable automatic manifest generation and use ours
if(MSVC)
    set_target_properties(WinOpAutoMouseKeybdtest PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/WinOpAutoMouseKeybdtest.manifest /MANIFESTUAC:NO"
    )
endif()

# Define install folder variable
set(INSTALL_FOLDER ${CMAKE_SOURCE_DIR}/test)

# Create install directory if it doesn't exist
file(MAKE_DIRECTORY ${INSTALL_FOLDER})

# Build to standard location and copy exe to install folder (avoids Debug/Release subfolders)
set_target_properties(WinOpAutoMouseKeybdtest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

# Copy executable and Python scripts to install folder after build
add_custom_command(TARGET WinOpAutoMouseKeybdtest POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy 
    $<TARGET_FILE:WinOpAutoMouseKeybdtest>
    ${INSTALL_FOLDER}/
    COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_SOURCE_DIR}/src/process_input.py
    ${INSTALL_FOLDER}/
    COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_SOURCE_DIR}/src/llm_handler.py
    ${INSTALL_FOLDER}/
    COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_SOURCE_DIR}/src/input_cleaner.py
    ${INSTALL_FOLDER}/
    COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_SOURCE_DIR}/src/LLM_config.json
    ${INSTALL_FOLDER}/
    COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_SOURCE_DIR}/src/system_prompt.md
    ${INSTALL_FOLDER}/
    COMMAND ${CMAKE_COMMAND} -E copy 
    ${CMAKE_SOURCE_DIR}/src/requirements.txt
    ${INSTALL_FOLDER}/
    COMMENT "Copying executable, Python scripts, and configuration files to install folder"
)