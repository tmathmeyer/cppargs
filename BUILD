load (
    "//rules/core/CPP/build_defs.py",
)

cpp_header (
  name = 'argparse',
  srcs = [
    "argparse.h"
  ],
)

cpp_binary (
  name = 'argparse_test',
  srcs = [
    'test.cpp'
  ],
  depends = [
    '//googletest:googletest',
    ':argparse',
  ],
  linkdirs = [
    'googletest/googletest/include',
    'googletest/googletest',
  ],
  flags = [
    '-lpthread'
  ]
)