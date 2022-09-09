filegroup(
  name = "path_bins",
  srcs = [":count-distinct"],
  visibility = ["//visibility:public"],
)

cc_binary(
  name = "count-distinct",
  srcs = [
    "app.c",
    "app.h",
    "main.c",
  ], 
  deps = [
    ":libhyperloglog",
    "//third-party:murmurhash2",
  ],
)

cc_library(
  name = "libhyperloglog",
  srcs = ["libhyperloglog.c"],
  hdrs = ["libhyperloglog.h"],
  deps = [
    "//data/serde",
  ],
  visibility = ["//visibility:public"],
)
