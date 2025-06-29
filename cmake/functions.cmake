function(target_copy_binaries)
  set(options "")
  set(oneValueArgs SOURCE_TARGET DEST_TARGET)
  set(multiValueArgs "")
  cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}"
                        ${ARGN})
  add_custom_command(
    TARGET ${ARGS_DEST_TARGET}
    POST_BUILD
    COMMAND
      "${CMAKE_COMMAND}" -E copy_if_different
      "$<TARGET_FILE_DIR:${ARGS_SOURCE_TARGET}>/$<TARGET_FILE_NAME:${ARGS_SOURCE_TARGET}>"
      "$<TARGET_FILE_DIR:${ARGS_DEST_TARGET}>"
    COMMENT
      "Copying '$<TARGET_FILE_DIR:${ARGS_SOURCE_TARGET}>/$<TARGET_FILE_NAME:${ARGS_SOURCE_TARGET}>' to '$<TARGET_FILE_DIR:${ARGS_DEST_TARGET}>'..."
  )
endfunction()
