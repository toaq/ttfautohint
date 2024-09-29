{
  description = "Fork of ttfautohint with Derani support";

  inputs = {
    flake-utils.url = github:numtide/flake-utils;
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in {
        defaultPackage = pkgs.stdenv.mkDerivation rec {
          name = "ttfautohint";
          version = "1.8.4";

          src = self;

          preAutoreconf = let
            printVersion = pkgs.writeShellScript "print-version" ''
              echo -n ${pkgs.lib.escapeShellArg version}
            '';
          in ''
            ./bootstrap --gnulib-srcdir=${pkgs.gnulib} --no-git --bootstrap-sync --skip-po
            cp ${printVersion} gnulib/git-version-gen
          '';

          postAutoreconf = ''
            substituteInPlace configure --replace "macx-g++" "macx-clang"
          '';

          nativeBuildInputs = [
            pkgs.pkg-config
            pkgs.autoreconfHook
            pkgs.flex
            pkgs.freetype
            pkgs.harfbuzz
            pkgs.libiconv
            pkgs.bison
            pkgs.perl
          ];

          configureFlags = [ "--with-qt=no" "--with-doc=no" ];

          # workaround https://github.com/NixOS/nixpkgs/issues/155458
          preBuild = pkgs.lib.optionalString pkgs.stdenv.cc.isClang ''
            rm version
          '';

          enableParallelBuilding = true;

          meta = {
            description = "Automatic hinter for TrueType fonts";
            mainProgram = "ttfautohint";
            longDescription = ''
              A library and two programs which take a TrueType font as the
              input, remove its bytecode instructions (if any), and return a
              new font where all glyphs are bytecode hinted using the
              information given by FreeType’s auto-hinting module.
            '';
            homepage = "https://www.freetype.org/ttfautohint";
            license = pkgs.lib.licenses.gpl2Plus; # or the FreeType License (BSD + advertising clause)
            platforms = pkgs.lib.platforms.unix;
          };
        };
      }
    );
}
