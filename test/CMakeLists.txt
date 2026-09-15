add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/yorm_test.hpp
    COMMAND yorm-gen ${CMAKE_CURRENT_SOURCE_DIR}/ ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libyang/modules/ ${CMAKE_CURRENT_SOURCE_DIR}/yorm-test.yang
    DEPENDS yorm-gen ${CMAKE_CURRENT_SOURCE_DIR}/yorm-test.yang
    COMMENT "Generating yorm_test.hpp for tests"
)

add_executable(yorm-test test_libyang_data_driver.cpp ${CMAKE_CURRENT_SOURCE_DIR}/yorm_test.hpp)

target_include_directories(yorm-test PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../submodules/libyang/src
    ${CMAKE_CURRENT_BINARY_DIR}/../submodules/libyang/libyang
)

target_link_libraries(yorm-test PRIVATE yang)
