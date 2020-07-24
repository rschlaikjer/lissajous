# Lissajous viewer

This tool provides a very basic implementation of an XY mode oscilloscope,
reading input data from a stereo audio file. The left and right audio amplitudes
are mapped to the X/Y offsets, and rendered to the screen.

By default, the scope display will render the last 100ms of points, with an
exponential decay function on the intensity of those points. For now, adjusting
the decay function and window size must be done directly in the source code.

When the program is running, there are a few hotkeys available:

- Space: Play / pause the playback of the data
- Left/Right: Scrub back/forward in time by 0.1 second
- Shift + Left/Right: Scrub back.forward in time by 1 second
- /: Toggle Left/Right X/Y mapping

![Example Output](/img/lissajous.gif)

## Building

This project depends on opengl and glfw for rendering, and libsndfile for audio
loading. To build on a debian based system, the following should suffice:

```
sudo apt install -y cmake build-essential libglfw3-dev libsndfile1-dev
mkdir build
cd build
cmake ../
make -j$(nproc)
```

Once you have `lissajous`, you should be able to run it against any stereo
audio file and have it load correctly. Note that this program maps the entire
file into memory - excessively large audio files may not be loadable on smaller
systems.
