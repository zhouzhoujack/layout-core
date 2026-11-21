find_package(GTest REQUIRED)

add_executable(ut_data
    main.cpp
    pointTest.cc
)

target_link_libraries(ut_data PRIVATE
    data
    GTest::gtest
)

add_test(NAME ut_data COMMAND ut_data)
