# ====================================================
# Import sub-projects
# ====================================================
add_subdirectory("Adesto/at25")

# ====================================================
# Public Headers
# ====================================================
set(PUB_HDR adesto_inc)
add_library(${PUB_HDR} INTERFACE)
target_include_directories(${PUB_HDR} INTERFACE "")
export(TARGETS ${PUB_HDR} FILE "${PROJECT_BINARY_DIR}/Adesto/${PUB_HDR}.cmake")

# ====================================================
# Common
# ====================================================
set(LINK_LIBS
  adesto_inc
  aurora_inc
  Boost::boost
  chimera_inc       # Chimera public headers
  prj_device_target
  prj_build_target # Compiler options for target device
)

set(LIB adesto_core)
add_library(${LIB} STATIC
  Adesto/common.cpp
)
target_link_libraries(${LIB} PRIVATE ${LINK_LIBS})
export(TARGETS ${LIB} FILE "${PROJECT_BINARY_DIR}/Adesto/${LIB}.cmake")
