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