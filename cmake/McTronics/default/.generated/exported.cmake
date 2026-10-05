set(DEPENDENT_MP_BIN2HEXMcTronics_default_yDBHIptY "/Applications/microchip/xc16/v2.10/bin/xc16-bin2hex")
set(DEPENDENT_DEPENDENT_TARGET_ELFMcTronics_default_yDBHIptY ${CMAKE_CURRENT_LIST_DIR}/../../../../out/McTronics/default.elf)
set(DEPENDENT_TARGET_DIRMcTronics_default_yDBHIptY ${CMAKE_CURRENT_LIST_DIR}/../../../../out/McTronics)
set(DEPENDENT_BYPRODUCTSMcTronics_default_yDBHIptY ${DEPENDENT_TARGET_DIRMcTronics_default_yDBHIptY}/${sourceFileNameMcTronics_default_yDBHIptY}.s)
add_custom_command(
    OUTPUT ${DEPENDENT_TARGET_DIRMcTronics_default_yDBHIptY}/${sourceFileNameMcTronics_default_yDBHIptY}.s
    COMMAND ${DEPENDENT_MP_BIN2HEXMcTronics_default_yDBHIptY} ${DEPENDENT_DEPENDENT_TARGET_ELFMcTronics_default_yDBHIptY} --image ${sourceFileNameMcTronics_default_yDBHIptY} ${addressMcTronics_default_yDBHIptY} ${modeMcTronics_default_yDBHIptY} -mdfp=/Users/nalyd/.mchp_packs/Microchip/PIC24F-KA-KL-KM_DFP/1.6.395/xc16 
    WORKING_DIRECTORY ${DEPENDENT_TARGET_DIRMcTronics_default_yDBHIptY}
    DEPENDS ${DEPENDENT_DEPENDENT_TARGET_ELFMcTronics_default_yDBHIptY})
add_custom_target(
    dependent_produced_source_artifactMcTronics_default_yDBHIptY 
    DEPENDS ${DEPENDENT_TARGET_DIRMcTronics_default_yDBHIptY}/${sourceFileNameMcTronics_default_yDBHIptY}.s
    )
