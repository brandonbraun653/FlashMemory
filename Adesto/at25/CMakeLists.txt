# ====================================================
# Common
# ====================================================
set(LINK_LIBS
  adesto_inc
  aurora_inc
  Boost::boost
  chimera_inc       # Chimera public headers
  prj_device_target # Compiler options for target device
)

# ====================================================
# Interface Library
# ====================================================
set(LIB lib_adesto_at25)
add_library(${LIB} STATIC
  at25_driver.cpp
)
target_link_libraries(${LIB} PRIVATE ${LINK_LIBS})
export(TARGETS ${LIB} FILE "${PROJECT_BINARY_DIR}/Adesto/${LIB}.cmake")
