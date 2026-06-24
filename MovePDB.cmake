# MovePDB.cmake - Renames an existing PDB so the linker can write a fresh one.
# The debugger keeps its file handle to the renamed file.

get_filename_component(PDBDir "${PDB_PATH}" DIRECTORY)
get_filename_component(PDBName "${PDB_PATH}" NAME)

set(HotReloadDir "${PDBDir}/HotReload")

if(EXISTS "${PDB_PATH}")
    # 2. Ensure the HotReload subdirectory exists
    if(NOT EXISTS "${HotReloadDir}")
        file(MAKE_DIRECTORY "${HotReloadDir}")
    endif()

    # 3. Generate the timestamped destination path inside the subfolder
    string(TIMESTAMP TimeStamp "%Y-%m-%d_%H-%M-%S")
    set(PDBDestination "${HotReloadDir}/${PDBName}")

    string(REGEX REPLACE "\\.pdb$" "_${TimeStamp}.pdb" PDBDestination "${PDBDestination}")

    # 4. Perform the move
    file(RENAME "${PDB_PATH}" "${PDBDestination}")
    message(STATUS "[HotReload] Moved ${PDB_PATH} -> ${PDBDestination}")
endif()
