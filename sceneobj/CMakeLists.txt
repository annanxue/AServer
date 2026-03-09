cmake_minimum_required(VERSION 3.15)

set(PROJBASE ${CMAKE_CURRENT_SOURCE_DIR}/..)


set(SCENEOBJ_ACTION_SOURCES
    action/state.cpp
    action/state_msg.cpp
    action/states.cpp
)

set(SCENEOBJ_OBJECT_SOURCES
    object/bezier_curve.cpp
    object/bullet.cpp
    object/camp_mng.cpp
    object/ctrl.cpp
    object/curve_mng.cpp
    object/item.cpp
    object/magic_area.cpp
    object/missile.cpp
    object/monster.cpp
    object/obj.cpp
    object/pick_point.cpp
    object/player.cpp
    object/player_mng.cpp
    object/snapshot.cpp
    object/spirit.cpp
    object/trigger.cpp
)

set(SCENEOBJ_UTILS_SOURCES
    sceneobj_utils/scene_obj_utils.cpp
)

set(SCENEOBJ_AI_SOURCES
    ai/action.cpp
    ai/action_ex.cpp
    ai/btnode.cpp
    ai/btree.cpp
    ai/common.cpp
    ai/component_ai.cpp
    ai/composite.cpp
    ai/condition.cpp
    ai/load_btree.cpp
    ai/threat_mgr.cpp
    ai/tinystr.cpp
    ai/tinyxml.cpp
    ai/tinyxmlerror.cpp
    ai/tinyxmlparser.cpp
    ai/utils.cpp
)

set(SCENEOBJ_SOURCES
    ${SCENEOBJ_ACTION_SOURCES}
    ${SCENEOBJ_OBJECT_SOURCES}
    ${SCENEOBJ_UTILS_SOURCES}
    ${SCENEOBJ_AI_SOURCES}
)

add_library(sceneobj STATIC ${SCENEOBJ_SOURCES})

target_include_directories(sceneobj PUBLIC
    ${PROJBASE}/lib/include
    ${PROJBASE}/lib/include/scene
    ${PROJBASE}/lib/include/detour
    ${PROJBASE}/lib/include/chipmunk
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/action
    ${CMAKE_CURRENT_SOURCE_DIR}/object
    ${CMAKE_CURRENT_SOURCE_DIR}/sceneobj_utils
    ${CMAKE_CURRENT_SOURCE_DIR}/ai
)

if(WIN32)
    target_compile_definitions(sceneobj PRIVATE
        _WIN32_WINNT=0x0601
        NOMINMAX
        _CRT_SECURE_NO_WARNINGS
        _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
        _CRT_NONSTDC_NO_DEPRECATE
    )
endif()

set_target_properties(sceneobj PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
    ARCHIVE_OUTPUT_DIRECTORY ${PROJBASE}/lib
)
