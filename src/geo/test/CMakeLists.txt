find_package(GTest REQUIRED)

add_executable(ut_geo
    main.cpp
    regionTest.cc
    booleanTest.cc
    gdsIOTest.cc
    oasisTest.cc
)

target_link_libraries(ut_geo PRIVATE
    reader
    db-io
    GTest::gtest
)

# reader 只提供格式注册（静态初始化），测试代码不直接引用其符号，
# 需要 --no-as-needed 防止链接器把它丢掉。
target_link_options(ut_geo PRIVATE "-Wl,--no-as-needed")

add_test(NAME ut_geo COMMAND ut_geo)
