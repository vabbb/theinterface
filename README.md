# The Interface

"The Interface" is a stacking Wayland compositor based on [wlroots](https://github.com/swaywm/wlroots).

For now it's just a programming excercise for me to learn C++. Most of the code is imported from [Rootston](https://github.com/emersion/rootston).

The objective in the long run is to make a full fledged Wayland compositor, capable of being used on a daily basis.

## Compiling
```bash
meson build
ninja -C build
```

## Running
```bash
./build/theinterface/theinterface -s "termite & thunar"
```
Replace `"termite & thunar"` with any program that you would like to run instead.
