# LASBlock

A tool to tile the .las file

## Dependencies

The code is based on the following prerequisites:
* LASlib(Windows)
* LASlib boost(Linux)

##  Compilation
prerequisites: cmake version>=3.0

```
1. git clone https://github.com/AndrewAndJenny/LASBlock.git
2. cd LASBlock
```

**in the cmake-bash**
* Windows
```
1. mkdir build && cd build
2. cmake .. -A x64
3. open the vs solution, and install
```
* Linux
```
1. mkdir build && cd build
2. cmake ..
3. make
```
A demo project is attached for test

## Supported File Format
* .las: ASPRS LAS file which contains LIDAR point data records
* .laz: ASPRS LAZ file which contains LIDAR point data records(compression)
## Functions
- [x] tile .las files according to sub-block size
- [x] tile .laz files according to sub-block size

##  Contact
If you found bugs or have new ideas,please pull requests:smile:
If you have trouble compiling or using this software,email to 15313326374@163.com
