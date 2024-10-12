{
  description = "Hyswipe";

  inputs = {
    systems = {
      type = "github";
      owner = "nix-systems";
      repo = "default-linux";
    };
    hyprland = {
      owner = "hyprwm";
      repo = "Hyprland";
      type = "github";
      inputs.systems.follows = "systems";
    };
  };

  outputs = {
    self,
    systems,
    hyprland,
    ...
  }: let
    inherit (builtins) concatStringsSep substring;
    inherit (hyprland.inputs) nixpkgs;

    perSystem = attrs:
      nixpkgs.lib.genAttrs (import systems) (system:
        attrs system (import nixpkgs {
          inherit system;
          overlays = [hyprland.overlays.hyprland-packages];
        }));

    # Generate version
    mkDate = longDate: (concatStringsSep "-" [
      (substring 0 4 longDate)
      (substring 4 2 longDate)
      (substring 6 2 longDate)
    ]);

    # TODO: get version from somewhere
    version = "0.0.0+date=${mkDate (self.lastModifiedDate or "19700101")}_${self.shortRev or "dirty"}";
  in {
    packages = perSystem (system: pkgs: {
      Hyswipe = let
        inherit (pkgs.lib) platforms subtractLists;
        hyprlandPkg = hyprland.packages.${system}.hyprland;
      in
        pkgs.gcc14Stdenv.mkDerivation {
          pname = "Hyswipe";
          inherit version;
          src = ./.;

          nativeBuildInputs = subtractLists (with pkgs; [meson ninja]) hyprlandPkg.nativeBuildInputs;
          buildInputs = [hyprlandPkg] ++ hyprlandPkg.buildInputs;
          dontUseCmakeConfigure = true;

          buildPhase = ''
            make all
          '';
          installPhase = ''
            mkdir -p $out/lib
            cp Hyswipe.so $out/lib/libHyswipe.so
          '';

          meta = {
            homepage = "https://github.com/KZDKM/Hyswipe";
            description = "Workspace swipe with mouse plugin for Hyprland";
            platforms = platforms.linux;
          };
        };
      default = self.packages.${system}.Hyswipe;
    });

    devShells = perSystem (system: pkgs: {
      default = pkgs.mkShell {
        name = "Hyswipe-shell";
        nativeBuildInputs = with pkgs; [gcc14];
        buildInputs = [hyprland.packages.${system}.hyprland];
        inputsFrom = [
          hyprland.packages.${system}.hyprland
          self.packages.${system}.Hyswipe
        ];
      };
    });

    formatter = perSystem (_: pkgs: pkgs.alejandra);
  };
}
