{
  pkgs,
  lib,
  inputs,
  ...
}:
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
    # Used for audio
    miniaudio 
    libpulseaudio
    alsa-lib
  ];
in
{
  # make dyn libs accessable in nixos
  env.LD_LIBRARY_PATH = lib.makeLibraryPath libs;
  # make openGL work on non-NixOS systems
  overlays = [ inputs.nixgl.overlay ];

  # install libs and additional programs
  packages =
    with pkgs;
    [
      cmake-format
      nixgl.nixGLIntel
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
      build && nixGLIntel ./build/supernova
    '';
  };

  # format on commit
  git-hooks.hooks.treefmt.enable = true;
  treefmt.config.programs = {
    clang-format.enable = true;
    cmake-format.enable = true;
  };
}
