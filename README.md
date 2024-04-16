# Hyswipe

Workspace swipe with mouse. Inspired by [Mac Mouse Fix](https://github.com/noah-nuebling/mac-mouse-fix).


## Installation

### Manual

To build, have hyprland headers installed and under the repo directory do:
```
make all
```
Then use `hyprctl plugin load` followed by the absolute path to the `.so` file to load, you could add this to your `exec-once` to load the plugin on startup

### Hyprpm
```
hyprpm add https://github.com/KZDKM/Hyswipe
hyprpm enable Hyswipe
```

## Config
- `plugin:hyswipe:button` which mouse button to enable swipe (will be unusable for other applications)
- `plugin:hyswipe:sensitivity`
- `plugin:hyswipe:lockCursor` lock cursor from moving when swiping


