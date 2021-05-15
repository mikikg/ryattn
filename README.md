# RyAttn
64 steps Audio Relay Attenuator with STM32F103 (BluePill) and SPI OLED display. 
Supports mechanical and optical rotary encoders from 2 to 2000+ imps/step via hardware QEI interface.
Configurable options and operation mode.

CMSIS, bare-metal, CMake, arm-none-eabi-gcc, CLion

For more information visit https://forum.yu3ma.net/thread-2589.html

# Build 

```
sudo apt install cmake git gcc-arm-none-eabi -y

git clone https://github.com/mikikg/ryattn

cd ryattn

rm -rf build/

mkdir build

cd build

cmake ..

make

```
