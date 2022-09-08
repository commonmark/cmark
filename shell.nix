{nixpkgs ? import <nixpkgs> {} }:
let
  inherit (nixpkgs) pkgs;

  nixPackages = [
    pkgs.clangStdenv
    pkgs.cmake
    pkgs.gdb
    pkgs.python3
    pkgs.perl
    pkgs.re2c
    pkgs.curl
  ];
in
pkgs.stdenv.mkDerivation {
  name = "env";
  buildInputs = nixPackages;
}
