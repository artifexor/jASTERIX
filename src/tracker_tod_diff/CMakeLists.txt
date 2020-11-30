
include_directories (
    "${CMAKE_CURRENT_LIST_DIR}"
    )

add_executable ( tracker_tod_diff "${CMAKE_CURRENT_LIST_DIR}/main.cpp")
target_link_libraries ( tracker_tod_diff jasterix)

target_link_libraries ( tracker_tod_diff
    ${Boost_LIBRARIES})

target_sources(tracker_tod_diff
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/toddifferencecalculator.h"
        "${CMAKE_CURRENT_LIST_DIR}/trackupdate.h"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/toddifferencecalculator.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/trackupdate.cpp"
    )
