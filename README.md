# Kinetic Codec

> For those that are busy I will cut to the chase. The code in this repository is not helpful for the application for which it is designed. I leave it here for the sake of scientific openness. The conclusion of my study was that nothing (in the space of linear static prediction models) beats delta encoding. I would recommend you simply implement delta encoding yourself (it just takes one minus sign).

This is source code to go along with the paper "Lossless Compression of Human Movement IMU Signals" published in the open access journal MDPI Sensors:
https://www.mdpi.com/1424-8220/20/20/5926

The `humoco` tool compresses IMU data using a linear predictive model and 16.16 fixed point mathematics. The fixed point mathematics guarantees identical results across diverse computation platforms and can be run on simple embedded hardware without floating point capabilities.

Special thanks to the following libraries:
[libfixmatrix](https://github.com/PetteriAimonen/libfixmatrix)
[libfixmath](https://code.google.com/archive/p/libfixmath/)

The public human movement IMU dataset HuGaDB is used to benchmark the performance of compression:
[paper](https://arxiv.org/abs/1705.08506), [dataset](https://github.com/romanchereshnev/HuGaDB)


## Usage

Code can be build using the bash script `build.sh`. cmake must be installed.


```
Usage: ./humoco [-t technique] [-k rice order] [-f filter_location] data_location
```
