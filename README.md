# About
A packet capture based scanner for Zenless Zone Zero. Used to quickly gather all your discs, engines and agents data from the game in a single click.

It can be used to quickly import all your data into [Zenless Optimizer](https://frzyc.github.io/zenless-optimizer/) or other tools that accept the same data format.

![](preview.png)
# Usage
- Download the latest version from the [releases page](https://github.com/AleXu224/zzz_packet_capture/releases)
- Extract the archive to a folder of your choice and start `zzz_packet_capture.exe`
  - The program needs to start with administrator privileges in order to capture packets from the game, you will automatically be prompted to do so
- Choose your region and click "Start Capture"
- Close Zenless Zone Zero if you haven't done so already
- Start the game and log in, the program will automatically capture your data (should be done by the time you see your character in game)
- Adjust your export settings if you want to, then click "Copy to clipboard" to get the exported data

# Disclaimer
This is gonna break every time a new version comes out. I am a am singlehandedly maintaining this and reverse engineering is not exactly my forte, so it might take a bit for me to get it working again. If you have experience and want to help your best bet is to help with updating [GracefulDumper](https://github.com/AleXu224/GracefulDumper) to the latest version of the game, since that is the main blocker.

If you are eager to help but have no experience then please don't hesitate to reach out and ask for how things are done, I might not be the best at it myself but I can certainly help you get started.

# Building

- Requirements:
  - [CMake](https://github.com/kitware/cmake)
  - A recent release of [Clang](https://github.com/llvm/llvm-project)
    - Other compilers might work but I rarely test them
  - [Ninja](https://github.com/ninja-build/ninja)
    - Optional, but recommended for faster builds
  - [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
  - Visual Studio or Visual Studio Build Tools
    - Make sure to install the "Desktop development with C++" workload
- Building:
  - Clone the repository and initialize the submodules:
  ```bash
  git clone https://github.com/AleXu224/zzz_packet_capture
  cd zzz_packet_capture
  git submodule update --init --recursive
  ```
  - Run cmake to generate the build files and install the dependencies (this will take a while, blame openssl for that):
  ```bash
  cmake -B build -G "Ninja Multi-Config" -DCMAKE_C_COMPILER="clang" -DCMAKE_CXX_COMPILER="clang++"
  
  cd build

  ninja
  # or if you want to build a release version
  ninja -f ./build-Release.ninja
  ```

# Credits
Massive thanks to the Reversed Rooms Discord for helping me with the reverse engineering. I wouldn't have been able to do this without them
