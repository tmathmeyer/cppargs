load("//rules/core/C/build_defs.py")

c_header (
  name = "argparse_h",
  srcs = [ "argparse.h" ],
)

cpp_binary (
  name = "argparse_test",
  srcs = [ "test.cpp" ],
  deps = [
    ":argparse_h",
    "//googletest:googletest",
    "//googletest:googletest_headers",
  ],
  include_dirs = [
    "googletest/googletest/include",
    "googletest/googletest",
  ],
  flags = [ "-lpthread" ],
)

cpp_test (
  name = "argparse_test2",
  srcs = [ "test.cpp" ],
  deps = [ ":argparse_h" ],
)