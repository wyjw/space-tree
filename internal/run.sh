LD_PRELOAD="../PerconaFT/build/ft/libft.so ../PerconaFT/build/portability/libtokuportability.so" ./fttest 40000 3200 1
LD_PRELOAD="../PerconaFT/build/ft/libft.so ../PerconaFT/build/portability/libtokuportability.so" ./fttest_block_alloc
LD_PRELOAD="../PerconaFT/build/ft/libft.so ../PerconaFT/build/portability/libtokuportability.so" ./fttest_bench

LD_PRELOAD="../PerconaInBlockFT/build/ft/libft.so ../PerconaInBlockFT/build/portability/libtokuportability.so" ./fttest 40000 3200 1
LD_PRELOAD="../PerconaInBlockFT/build/ft/libft.so ../PerconaInBlockFT/build/portability/libtokuportability.so" ./fttest_block_alloc
LD_PRELOAD="../PerconaInBlockFT/build/ft/libft.so ../PerconaInBlockFT/build/portability/libtokuportability.so" ./fttest_bench

LD_PRELOAD="../PerconaWithUringFT/build/ft/libft.so ../PerconaWithUringFT/build/portability/libtokuportability.so" ./fttest 40000 3200 1
LD_PRELOAD="../PerconaWithUringFT/build/ft/libft.so ../PerconaWithUringFT/build/portability/libtokuportability.so" ./fttest_block_alloc
LD_PRELOAD="../PerconaWithUringFT/build/ft/libft.so ../PerconaWithUringFT/build/portability/libtokuportability.so" ./fttest_bench
