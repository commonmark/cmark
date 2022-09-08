with (import <nixpkgs> {});
mkShell {
  buildInputs = [
    clangStdenv
    cmake
    gdb
    python3
    perl
    re2c
    curl
  ];
}
