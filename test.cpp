#include <string>
#include <cstring>
#include <iostream>

#include "argparse.h"
#include "gtest/gtest.h"

using namespace argparse;
char *CONST_BINARY = (char *)"bin";

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

template<typename X, typename ...Y>
X parse(std::vector<std::string> inargs) {
    char **args = new char*[inargs.size() + 1];
    args[0] = CONST_BINARY;
    for(size_t i=0; i<inargs.size(); i++) {
        args[i+1] = new char[inargs[i].size() + 1];
        std::strcpy(args[i+1], inargs[i].c_str());
    }
    return *dynamic_cast<X *>(ParseArgs<X, Y...>(inargs.size() + 1, args));
}

template <char... Digits>
auto operator"" _i() {
	return argparse::tuple_index::index<tuple_index::parse<Digits...>()>{};
}

TEST(FlagsWorkAsExpected, CreateFlag) {
    Flag(Example, "--example", "-e", "An extended description of 'Example'");
    EXPECT_STREQ(_F_Example().full.c_str(), "--example");
    EXPECT_STREQ(_F_Example().simple.c_str(), "-e");
    EXPECT_STREQ(_F_Example().desc.c_str(), "An extended description of 'Example'");
}

TEST(ParserParamTest, NoArgsAtAll) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example);
    ASSERT_THROW(parse<Example>({""}), TraceException);
}

TEST(ParserParamTest, NoArgsType) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example);
    Example e = parse<Example>({"--example"});
}

TEST(ParserParamTest, SimpleStringType) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example, std::string);
    Example e = parse<Example>({"--example", "val"});
    ASSERT_STREQ(e[0_i].c_str(), "val");
}

TEST(ParserParamTest, TestIntegralTypes) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example, int);
    Example e = parse<Example>({"--example", "5"});
    ASSERT_EQ(e[0_i], 5);
}

TEST(ParserParamTest, TestFloatingPointTypes) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example, double);
    Example e = parse<Example>({"--example", "5.5"});
    ASSERT_EQ(e[0_i], 5.5);
}

TEST(ParserParamTest, TestOptionalType) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example, std::optional<int>);
    
    Example a = parse<Example>({"--example", "5"});
    ASSERT_EQ(a[0_i], 5);
    
    Example b = parse<Example>({"--example"});
    ASSERT_EQ(b[0_i], std::nullopt);
}

TEST(ParserParamTest, TestTupleType) {
    Flag(Example, "--example", "-e", "Description");
    Arg(Example, std::tuple<int, double>);
    
    Example e = parse<Example>({"--example", "5", "5.5"});
    ASSERT_EQ(std::get<0>(e[0_i]), 5);
    ASSERT_EQ(std::get<1>(e[0_i]), 5.5);
}

TEST(ParserParamTest, TestNestedTypeType) {
    Flag(Nested, "--nested", "-n", "Description");
    Flag(Example, "--example", "-e", "Description");

    Arg(Nested, int);
    Arg(Example, Nested);
    
    Example e = parse<Example>({"--example", "--nested", "4"});
    ASSERT_EQ(e[0_i][0_i], 4);
}

TEST(ParserParamTest, TestAnyOrderType) {
    Flag(N1, "--N1", "-A", "Description");
    Flag(N2, "--N2", "-B", "Description");
    Flag(Example, "--example", "-e", "Description");
    
    Arg(N1, int);
    Arg(N2, int);
    Arg(Example, AnyOrder<N1, N2>);
    
    Example a = parse<Example>({"--example", "--N1", "1", "--N2", "2"});
    ASSERT_EQ(a[0_i][0_i][0_i], 1);
    ASSERT_EQ(a[0_i][1_i][0_i], 2);

    Example b = parse<Example>({"--example", "--N2", "2", "--N1", "1"});
    ASSERT_EQ(b[0_i][0_i][0_i], 1);
    ASSERT_EQ(b[0_i][1_i][0_i], 2);

    ASSERT_THROW(parse<Example>({"--example", "--N2", "2"}), TraceException);
}

TEST(ParserParamTest, TestAnyOrderOptionalType) {
    Flag(N1, "--N1", "-A", "Description");
    Flag(N2, "--N2", "-B", "Description");
    Flag(Example, "--example", "-e", "Description");
    
    Arg(N1, int);
    Arg(N2, int);
    Arg(Example, AnyOrder<std::optional<N1>, N2>);
    
    Example a = parse<Example>({"--example", "--N1", "1", "--N2", "2"});
    ASSERT_EQ(a[0_i][0_i].value()[0_i], 1);
    ASSERT_EQ(a[0_i][1_i][0_i], 2);

    Example b = parse<Example>({"--example", "--N2", "2", "--N1", "1"});
    ASSERT_EQ(b[0_i][0_i].value()[0_i], 1);
    ASSERT_EQ(b[0_i][1_i][0_i], 2);
    
    Example c = parse<Example>({"--example", "--N2", "2"});
    ASSERT_FALSE(c[0_i][0_i].has_value());
    ASSERT_EQ(c[0_i][1_i][0_i], 2);
}

TEST(ParserParamTest, TestOptionalTupleType) {
    Flag(Example, "--example", "-e", "Description");
    
    Arg(Example, std::optional<std::tuple<int, int, std::string>>);
    
    Example a = parse<Example>({"--example", "1", "2", "foo"});
    std::tuple<int, int, std::string> val = std::make_tuple(1, 2, "foo");
    ASSERT_EQ(a[0_i].value(), val);
    
    Example b = parse<Example>({"--example"});
    ASSERT_FALSE(b[0_i].has_value());

    // Args "1" and "2" are left over, raising the exception
    ASSERT_THROW(parse<Example>({"--example", "1", "2"}), TraceException);
}

TEST(ParserParamTest, TestAnyOrderOptionalTupleType) {
    Flag(Example, "--example", "-e", "Description");
    Flag(N1, "--N1", "-A", "Description");
    Flag(N2, "--N2", "-B", "Description");
    
    Arg(N1);
    Arg(N2);
    Arg(Example, AnyOrder<std::optional<std::tuple<int, int>>,
                          std::optional<std::tuple<N1, N2>>>);
    
    Example a = parse<Example>({"--example"});
    ASSERT_FALSE(a[0_i][0_i].has_value());
    ASSERT_FALSE(a[0_i][1_i].has_value());
    
    Example b = parse<Example>({"--example", "1", "2"});
    std::tuple<int, int> b1 = std::make_tuple(1, 2);
    ASSERT_EQ(b[0_i][0_i].value(), b1);
    ASSERT_FALSE(b[0_i][1_i].has_value());

    Example c = parse<Example>({"--example", "--N1", "--N2"});
    ASSERT_FALSE(c[0_i][0_i].has_value());
    ASSERT_TRUE(c[0_i][1_i].has_value());

    Example d = parse<Example>({"--example", "--N1", "--N2", "1", "2"});
    std::tuple<int, int> d1 = std::make_tuple(1, 2);
    ASSERT_EQ(d[0_i][0_i].value(), d1);
    ASSERT_TRUE(d[0_i][1_i].has_value());
    
    Example e = parse<Example>({"--example", "1", "2", "--N1", "--N2"});
    std::tuple<int, int> e1 = std::make_tuple(1, 2);
    ASSERT_EQ(e[0_i][0_i].value(), e1);
    ASSERT_TRUE(e[0_i][1_i].has_value());
}

TEST(ParserParamTest, TestNamedParameters) {
    Flag(Example, "--example", "-e", "Description");
    NamedType(FOO, int);
    NamedType(BAR, int);
    Arg(Example, FOO, BAR);

    Example a = parse<Example>({"--example", "1", "2"});
    int foo = a[0_i];
    int bar = a[1_i];
    ASSERT_EQ(foo, 1);
    ASSERT_EQ(bar, 2);

    std::ostringstream stream;
    DisplayHelp<Example>(stream);
    ASSERT_EQ(stream.str(), "\033[1m-e, --example\033[0m FOO<int>, BAR<int>\n\tDescription\n\n");
}
