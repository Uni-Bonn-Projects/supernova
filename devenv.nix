{ pkgs, lib, ... }:
let
  # libs required by framework
  libs = with pkgs; [
    libGL
    libX11
    libffi
    libxcursor
    libxi
    libxinerama
    libxkbcommon
    libxrandr
    pkg-config
    wayland
    wayland-scanner
  ];
in
{
  # make dyn libs accessable in nixos
  env.LD_LIBRARY_PATH = lib.makeLibraryPath libs;

  # install libs and additional programs
  packages =
    with pkgs;
    [
      cmake-format
    ]
    ++ libs;

  # install c++ relevant tools
  languages.cplusplus.enable = true;

  scripts = {
    build.exec = ''
      if [ ! -d build/ ]; then
        cmake -S . -B build/
      fi
      cmake --build build/
    '';
    run.exec = ''
      build && ./build/supernova
    '';
  };

  # format on commit
  git-hooks.hooks.treefmt.enable = true;
  treefmt.config.programs = {
    clang-format.enable = true;
    cmake-format.enable = true;
  };
}
