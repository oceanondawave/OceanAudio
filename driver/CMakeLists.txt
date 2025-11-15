if(NOT MSVC)
    message(STATUS "Skipping driver targets: requires MSVC/WDK toolchain")
    return()
endif()

add_subdirectory(service)
# Kernel-mode driver project uses the Windows Driver Kit and will be added once
# the WDK build integration is configured.

