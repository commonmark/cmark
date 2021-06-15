// swift-tools-version:5.3
// The swift-tools-version declares the minimum version of Swift required to build this package.

import PackageDescription

let package = Package(
    name: "cmark-gfm",
    products: [
        // Products define the executables and libraries a package produces, and make them visible to other packages.
        .library(
            name: "cmark-gfm",
            targets: ["cmark-gfm"]),
        .library(
            name: "cmark-gfm-extensions",
            targets: ["cmark-gfm-extensions"]),
        .executable(
            name: "cmark-gfm-bin",
            targets: ["cmark-gfm-bin"]),
        .executable(name: "api_test",
            targets: ["api_test"])
    ],
    targets: [
        .target(name: "cmark-gfm",
          path: "src",
          exclude: [
            "scanners.re",
            "libcmark-gfm.pc.in",
            "config.h.in",
            "CMakeLists.txt",
            "cmark-gfm_version.h.in",
            "case_fold_switch.inc",
            "entities.inc",
          ]
        ),
        .target(name: "cmark-gfm-extensions",
          dependencies: [
            "cmark-gfm",
          ],
          path: "extensions",
          exclude: [
            "CMakeLists.txt",
            "ext_scanners.re",
          ]
        ),
        .target(name: "cmark-gfm-bin",
          dependencies: [
            "cmark-gfm",
            "cmark-gfm-extensions",
          ],
          path: "bin",
          sources: [
            "main.c",
          ]
        ),
        .target(name: "api_test",
          dependencies: [
            "cmark-gfm",
            "cmark-gfm-extensions",
          ],
          path: "api_test",
          exclude: [
            "CMakeLists.txt",
          ]
        )
    ]
)
