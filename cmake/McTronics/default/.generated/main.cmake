include("${CMAKE_CURRENT_LIST_DIR}/rule.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/file.cmake")

set(McTronics_default_library_list )

# Handle files with suffix s, for group default-XC16
if(McTronics_default_default_XC16_FILE_TYPE_assemble)
add_library(McTronics_default_default_XC16_assemble OBJECT ${McTronics_default_default_XC16_FILE_TYPE_assemble})
    McTronics_default_default_XC16_assemble_rule(McTronics_default_default_XC16_assemble)
    list(APPEND McTronics_default_library_list "$<TARGET_OBJECTS:McTronics_default_default_XC16_assemble>")

endif()

# Handle files with suffix S, for group default-XC16
if(McTronics_default_default_XC16_FILE_TYPE_assemblePreproc)
add_library(McTronics_default_default_XC16_assemblePreproc OBJECT ${McTronics_default_default_XC16_FILE_TYPE_assemblePreproc})
    McTronics_default_default_XC16_assemblePreproc_rule(McTronics_default_default_XC16_assemblePreproc)
    list(APPEND McTronics_default_library_list "$<TARGET_OBJECTS:McTronics_default_default_XC16_assemblePreproc>")

endif()

# Handle files with suffix c, for group default-XC16
if(McTronics_default_default_XC16_FILE_TYPE_compile)
add_library(McTronics_default_default_XC16_compile OBJECT ${McTronics_default_default_XC16_FILE_TYPE_compile})
    McTronics_default_default_XC16_compile_rule(McTronics_default_default_XC16_compile)
    list(APPEND McTronics_default_library_list "$<TARGET_OBJECTS:McTronics_default_default_XC16_compile>")

endif()

# Handle files with suffix s, for group default-XC16
if(McTronics_default_default_XC16_FILE_TYPE_dependentObject)
add_library(McTronics_default_default_XC16_dependentObject OBJECT ${McTronics_default_default_XC16_FILE_TYPE_dependentObject})
    McTronics_default_default_XC16_dependentObject_rule(McTronics_default_default_XC16_dependentObject)
    list(APPEND McTronics_default_library_list "$<TARGET_OBJECTS:McTronics_default_default_XC16_dependentObject>")

endif()

# Handle files with suffix elf, for group default-XC16
if(McTronics_default_default_XC16_FILE_TYPE_bin2hex)
add_library(McTronics_default_default_XC16_bin2hex OBJECT ${McTronics_default_default_XC16_FILE_TYPE_bin2hex})
    McTronics_default_default_XC16_bin2hex_rule(McTronics_default_default_XC16_bin2hex)
    list(APPEND McTronics_default_library_list "$<TARGET_OBJECTS:McTronics_default_default_XC16_bin2hex>")

endif()

# Handle files with suffix elf, for group default-XC16
if(McTronics_default_default_XC16_FILE_TYPE_objcopy_lss)
add_library(McTronics_default_default_XC16_objcopy_lss OBJECT ${McTronics_default_default_XC16_FILE_TYPE_objcopy_lss})
    McTronics_default_default_XC16_objcopy_lss_rule(McTronics_default_default_XC16_objcopy_lss)
    list(APPEND McTronics_default_library_list "$<TARGET_OBJECTS:McTronics_default_default_XC16_objcopy_lss>")

endif()


# Main target for this project
add_executable(McTronics_default_image_yDBHIptY ${McTronics_default_library_list})

set_target_properties(McTronics_default_image_yDBHIptY PROPERTIES
    OUTPUT_NAME "default"
    SUFFIX ".elf"
    RUNTIME_OUTPUT_DIRECTORY "${McTronics_default_output_dir}")
target_link_libraries(McTronics_default_image_yDBHIptY PRIVATE ${McTronics_default_default_XC16_FILE_TYPE_link})
# Add the link options from the rule file.
McTronics_default_link_rule( McTronics_default_image_yDBHIptY)

# Call bin2hex function from the rule file
McTronics_default_bin2hex_rule(McTronics_default_image_yDBHIptY)

#Add objcopy steps
McTronics_default_objcopy_lss_rule(McTronics_default_image_yDBHIptY)

