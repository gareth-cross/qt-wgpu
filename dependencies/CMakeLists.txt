function(add_dawn)
  set(DAWN_FETCH_DEPENDENCIES OFF)
  set(DAWN_BUILD_SAMPLES OFF)
  set(DAWN_USE_X11 OFF)
  set(DAWN_USE_GLFW OFF)
  set(DAWN_ENABLE_DESKTOP_GL OFF)
  set(DAWN_ENABLE_OPENGLES OFF)
  if (UNIX AND NOT APPLE)
    set(DAWN_USE_WAYLAND ON)
  endif()
  set(TINT_BUILD_CMD_TOOLS OFF)
  set(TINT_BUILD_TESTS OFF)
  set(TINT_BUILD_IR_BINARY OFF)
  if(NOT TARGET webgpu_dawn)
    add_subdirectory(dawn)
  endif()
endfunction()
add_dawn()

function(add_libfmt)
  set(FMT_TEST OFF)
  if(NOT TARGET fmt::fmt-header-only)
    add_subdirectory(fmt EXCLUDE_FROM_ALL)
  endif()
endfunction()
add_libfmt()

function(add_magic_enum)
  if(NOT TARGET magic_enum::magic_enum)
    add_subdirectory(magic_enum)
  endif()
endfunction()
add_magic_enum()
