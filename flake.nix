{
    description = "Interpreter of IGCSE Computer Science 0478 pseudocode";

    inputs = {
        nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
        flake-utils.url = "github:numtide/flake-utils";
    };

    outputs = { self, nixpkgs, flake-utils }: flake-utils.lib.eachDefaultSystem (system: rec {
        packages = {
            pcse = with nixpkgs.legacyPackages."${system}"; stdenv.mkDerivation {
                pname = "pcse";
                version = "v0.0.3";
                src = ./.;
                nativeBuildInputs = [
                    cmake
                ] ++ (with llvmPackages_13; [
                    clang
                ]);
                buildInputs = [
                    catch2
                ];
                buildPhase = ''
                    cmake -DCMAKE_BUILD_TYPE=Release .
                    make
                '';
                installPhase = ''
                    mkdir -p $out/bin
                    cp pcse $out/bin
                '';
            };
        };

        defaultPackage = packages.pcse;
        defaultApp = { 
            type = "app";
            program = "bin/pcse";
        };
    });
}
