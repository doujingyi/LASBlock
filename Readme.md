# LASBlock

A tool to tile the .las file

## Dependencies

The code is based on the following prerequisites:
* LASlib

##  Compilation
prerequisites: cmake version>=3.0

```
1. git clone https://github.com/AndrewAndJenny/LASBlock.git
2. cd LASBlock
```

**in the cmake-bash**
```
3. mkdir build
4. cmake .. -A x64
5. open the vs solution, and install
```
A demo project is attached for test

## Supported File Format
* .las: ASPRS LAS file which contains LIDAR point data records

## Functions
- [x] tile .las files according to sub-block size
- [x] tile .laz files according to sub-block size

##  Contact
If you found bugs or have new ideas,please pull requests:smile:
If you have trouble compiling or using this software,email to 15313326374@163.com