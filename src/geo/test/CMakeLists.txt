find_package(GTest REQUIRED)

add_executable(ut_geo
    main.cpp
    regionTest.cc
    booleanTest.cc
    gdsIOTest.cc
    oasisTest.cc
)

target_link_libraries(ut_geo PRIVATE
    db-io
    GTest::gtest
)

add_test(NAME ut_geo COMMAND ut_geo)
