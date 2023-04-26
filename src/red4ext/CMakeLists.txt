include(Red4ext)

target_link_libraries(let_there_be_flight.dll
  PUBLIC
    Detours
    FMOD
    RED4ext.SDK
    spdlog
    Codeware
    PhysX3
)