add_library(rakhook STATIC)
add_library(rakhook::rakhook ALIAS rakhook)

include_directories(${CMAKE_SOURCE_DIR}/deps/polyhook)

target_include_directories(rakhook PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(rakhook PUBLIC cxx_std_20)
target_link_libraries(rakhook PUBLIC PolyHook_2)

target_sources(rakhook PRIVATE
    "RakHook/offsets.cpp"
    "RakHook/rakhook.cpp"
    "RakHook/samp.cpp"

    "RakNet/BitStream.cpp"
    "RakNet/DS_HuffmanEncodingTree.cpp"
    "RakNet/PluginInterface.cpp"
    "RakNet/StringCompressor.cpp"
)

set_property(TARGET rakhook PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")