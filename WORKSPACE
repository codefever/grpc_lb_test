workspace(name = 'grpc_lb_test')

load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")
load("@bazel_tools//tools/build_defs/repo:git.bzl", "git_repository")

git_repository(
    name = "com_github_grpc_grpc",
    remote = "https://github.com/grpc/grpc.git",
    commit = "cc43fd64ab97e45fcdd38759dd8d9eb273823b9e",
)
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()

load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps()

git_repository(
    name   = "com_google_gflags",
    tag    = "v2.2.0",
    remote = "https://github.com/gflags/gflags.git"
)
bind(
    name   = "gflags",
    actual = "@com_google_gflags//:gflags",
)

git_repository(
    name = "com_google_absl",
    remote = "https://github.com/abseil/abseil-cpp.git",
    tag = "20190808",
)

http_archive(
    name = "com_google_glog",
    url = "https://github.com/google/glog/archive/v0.4.0.tar.gz",
    strip_prefix = "glog-0.4.0",
)
bind(
    name   = "glog",
    actual = "@com_google_glog//:glog",
)
