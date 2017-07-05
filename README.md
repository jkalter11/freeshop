![freeShop](https://notabug.org/arc13/freeShop/raw/master/res/app/banner.png)

Open source eShop alternative for the Nintendo 3DS. Allows you to browse and install titles you own (i.e. titles for which you have the titlekey).

### Usage Instructions

1. Install the CIA file on your CFW of choice (preferably one removing region restrictions)

2. Put an encTitleKeys.bin file in the directory `sdmc:/3ds/data/freeShop/keys/` (or go to update settings in freeShop and add a URL to download)

3. Launch freeShop and enjoy

### Building Instructions

freeShop depends on the [cpp3ds library](https://github.com/Naxann/cpp3ds), whose location must be defined
in the environment variable CPP3DS. Therefore, freeShop also needs cpp3ds's
dependencies as well.

To build from source on a Linux/UNIX-based system, run the following commands:

	mkdir -p build
	cd build
	cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EMULATOR=OFF -DBUILD_TESTS=OFF ..
	make -j4

To build the emulator or unit tests, simply change the relevant flags from "OFF" to "ON".

### Credit & Thanks

All the people that helped make this possible [listed here](https://notabug.org/arc13/freeShop/src/master/CREDITS.md).
