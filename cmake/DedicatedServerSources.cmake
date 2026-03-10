set(MINECRAFT_DEDICATED_SERVER_SOURCES ${MINECRAFT_CLIENT_SOURCES})

# Remove the client Win32 entry point and the Windows .rc resource file
list(REMOVE_ITEM MINECRAFT_DEDICATED_SERVER_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Windows64/Windows64_Minecraft.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Client/Xbox/MinecraftWindows.rc"
)

# Add the dedicated-server entry point
list(APPEND MINECRAFT_DEDICATED_SERVER_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/Minecraft.Server/DedicatedMain.cpp"
)
