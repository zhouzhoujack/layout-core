find_package(GTest REQUIRED)

add_executable(ut_utils
    main.cpp
    stringTest.cc
    variantTest.cc
)

target_link_libraries(ut_utils PRIVATE
    utils
    GTest::gtest
)

add_test(NAME ut_utils COMMAND ut_utils)
