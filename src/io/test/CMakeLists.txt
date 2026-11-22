find_package(GTest REQUIRED)

add_executable(ut_io
    main.cpp
    ioTest.cc
)

target_link_libraries(ut_io PRIVATE
    io
    GTest::gtest
)

add_test(NAME ut_io COMMAND ut_io)
