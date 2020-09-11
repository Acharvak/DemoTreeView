cmake_minimum_required(VERSION 3.5)

project(TreeDemo LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets LinguistTools Xml REQUIRED)

file(STRINGS source_files.txt PROJECT_FILES)

add_custom_target(update_ts lupdate "@source_files.txt" -ts TreeDemo_ru_RU.ts
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

add_custom_command(
    MAIN_DEPENDENCY TreeDemo_ru_RU.ts
    DEPENDS update_ts
    OUTPUT TreeDemo_ru_RU.qm
    COMMAND lrelease "${CMAKE_CURRENT_SOURCE_DIR}/TreeDemo_ru_RU.ts" "-qm" TreeDemo_ru_RU.qm
)

add_executable(TreeDemo
    ${PROJECT_FILES}
    TreeDemo_ru_RU.qm
)

add_dependencies(TreeDemo update_ts)

target_link_libraries(TreeDemo PRIVATE Qt5::Widgets Qt5::Xml)

# qt5_create_translation сломана
#qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
