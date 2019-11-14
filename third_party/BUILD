licenses(["notice"])  # 3-clause BSD
package(default_visibility = ["//visibility:public"])

load("@rules_foreign_cc//tools/build_defs:configure.bzl", "configure_make")

# https://github.com/envoyproxy/envoy/blob/master/bazel/foreign_cc/BUILD
configure_make(
    name = "gperftools_build",
    configure_options = [
        "--enable-shared=no",
        "--enable-frame-pointers",
        "--disable-libunwind",
    ],
    lib_source = "@com_github_gperftools_gperftools//:all",
    linkopts = ["-lpthread"],
    make_commands = ["make install-libLTLIBRARIES install-perftoolsincludeHEADERS"],
    static_libraries = select({
        "//conditions:default": ["libtcmalloc_and_profiler.a"],
    }),
)

cc_library(
    name = "gperftools",
    deps = [
        ":gperftools_build",
    ],
)