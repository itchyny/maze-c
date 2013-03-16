<!--
File: COMPRESS_SPEC0.md
Author: itchyny
Last Change: 2013/03/17 00:59:05.
-->

# Maze Compression Specification (version 0)
This document defines how to compress/decompress a maze.
The program `cam` works as this document states.
The version of this compression spec is 0.
This document does not state the detail of other versions.

## Compression Layers
There are 4 layers as follow.

### The 1st layer: A maze
A maze consists of the following information.

        a maze:
        sizex :: uint32_t, sizey :: uint32_t
        maze[sizex][sizey] :: [[uint8_t]]

The parameters `sizex` and `sizey` defines the size of the maze.
The type of these parameters is `uint32_t`.
The two dimensional array `maze` is an array representing the maze.
Each element is an `uint8_t` value.


An element of a maze is an `uint8_t` value.
Especially, following values are defined.

        undesided = 0
        wall = 1
        road = 2
        start = 3
        goal = 4

Other values are interpreted as same as `undesided`.
Therefore the total number of all possible mazes are

        5 ** (sizex * sizey).

In most cases, each of `start` and `goal` appears one times and almost every 
element is either a `wall` or a `road`.
Thus there are only about

        2 ** (sizex * sizey)

mazes.

### The 2nd layer
Data of this layer is an one-dimensional array of `uint8_t`.

        data[] :: [uint8_t]

This `data` is composed of two regions: frame region and data region.


Firstly, the frame region of `data` consists various information on the maze.
One frame has a fixed size 9.
And it has three parts.

        a frame:
        id :: uint8_t
        num1 :: [uint8_t, uint8_t, uint8_t, uint8_t]
        num2 :: [uint8_t, uint8_t, uint8_t, uint8_t]

The number `num1` and `num2` are stored in little-endian.
In this version, following frames are defined.

        UNDESIDED:    id = 1, num1 = posx, num2 = posy
        START:        id = 2, num1 = posx, num2 = posy
        GOAL:         id = 3, num1 = posx, num2 = posy
        ID:           id = 187, num1 = MAZEID, num2 = VERSION
        SIZE:         id = 'Z' = 90, num1 = SIZEX, num2 = SIZEY
        FLAG:         id = 'F' = 70, num1 = flagid1, num2 = flagid2
        ENDOFFRAME:   id = 0, num1 = 0, num2 = 0

where we define

        MAZEID = (251 | (238 << 8) | (190 << 16) | (187 << 24))
        VERSION = ((MAJOR_VERSION << 24) | (MINOR_VERSION << 16)
               | (REVISION_VERSION << 8) | COMPRESS_VERSION)

The `UNDESIDED`, `START` and `GOAL` frames are stored for the exceptional 
blocks in the maze.

The `ID` frame is composed by `MAZEID`, `maze` version and compression version.
When the program `maze` try to decompress a data, it compares the compression 
version in the data and that of the program.
In case the compression version in the data is larger, it warns that it may 
not be completely decompressed.

The `SIZE` frame is composed by `SIZEX` and `SIZEY`.

The `FLAG` frame is composed by `flagid1` and `flagid2`
These numbers  are defined by the following relations.

        flagid1 = (aflag ? 2 : 0) | (nflag ? 1 : 0)
        flagid2 = 0

The flags `aflag` and `nflag` are used if these options are not specified for 
the decompressing program.
For example, the command

        maze -u `maze -c -a`

stores the flag `-a` so this result in alphabetical output.
However, the command

        maze -nu `maze -c -a`

is specified the flag `-n` so this result in numerical output.
In order to disable both of `-a` and `-n`, use

        maze -eu `maze -c -a`


The `END` frame indicates the end of the frame.
It is followed by the main data of the maze.



In the data region, the main body of the maze if stored.
Firstly, the `wall`s are regarded as `0` and otherwise `1`.
Secondly, store each 8 elements into a `uint8_t` number.
For example, the 8 elements of a maze

        XX__XXXXXX__XXXX

is stored to a number `187`.


There are at least 4 frames (id, size, start and goal) so the length of the
data results in about

        4 * 9 + sizex * sizey / 8

Thus the number of possible data is about

        256 ** (4 * 9 + sizex * sizey / 8)

### The 3rd layer: all numbers are in 0-61
In this layer, all the numbers are stored in one of `0` to `61`.
The object is to store the data to a numbers and alphabets.


Compression from 2nd layer to 3rd layer is done by following procedure.

1. If the data starts with `0, 0, 0, 0` then store `60` and go next.
2. If the data starts with `0, 0, 0` then store `59` and go next.
3. If the data starts with `255, 255, 255, 255` then store `62` and go next.
4. If the data starts with `x, y`, where the pair `(x, y)` is one of the
   following list, then store `i` and go next.

        i, x, y              i, x, y             i, x, y
        34, 0, 0             43, 187, 251        52, 238, 250
        35, 174, 187         44, 190, 187        53, 238, 251
        36, 174, 251         45, 190, 238        54, 239, 174
        37, 186, 187         46, 235, 238        55, 239, 238
        38, 187, 187         47, 238, 174        56, 251, 187
        39, 187, 190         48, 238, 186        57, 251, 238
        40, 187, 235         49, 238, 187        58, 255, 255
        41, 187, 238         50, 238, 190
        42, 187, 239         51, 238, 238


5. If the data starts with `x`, where the number `x` is one of the following
list, then store `i` and go next.

        i, x          i, x          i, x          i, x
        0, 0          8, 64         16, 170       24, 234
        1, 1          9, 65         17, 171       25, 235
        2, 4          10, 68        18, 174       26, 238
        3, 5          11, 69        19, 175       27, 239
        4, 16         12, 80        20, 186       28, 250
        5, 17         13, 81        21, 187       29, 251
        6, 20         14, 84        22, 190       30, 254
        7, 21         15, 85        23, 191       31, 255

6. If the data starts with `x`, where the number `x` is less than `33`, then
store `33, x` and go next.
7. Otherwise, store `32, x & 31, (x >> 5) & 31` and go next.


The length of this layer depends on the maze.
In most cases, the length is a little smaller than the 2nd layer.
Thus the approximate possible number of data is about

        62 ** (4 * 9 + sizex * sizey / 8)

### The 4rd layer: alphabets or numbers
This layer is the result of compression.
All characters are one of alphabets or numbers.

There is an one-to-one correspondence to the 3rd layer.

        3rd layer  4th layer
        0          '0'
        1          '1'
        ...        ...
        9          '9'
        10         'A'
        11         'B'
        ...        ...
        35         'Z'
        36         'a'
        37         'b'
        ...        ...
        61         'z'

Thus the length of the compression result is the same as the 3rd layer.

## Example

         $ maze -aCd -w 5 -h 5

          XXXXXXXXXXXXXXXXXXXXXX
          S                   XX
          XXXXXX  XXXXXXXXXX  XX
          XX  XX      XX      XX
          XX  XX  XX  XX  XXXXXX
          XX      XX  XX      XX
          XX  XXXXXXXXXXXXXX  XX
          XX  XX      XX  XX  XX
          XX  XX  XXXXXX  XX  XX
          XX  XX               G
          XXXXXXXXXXXXXXXXXXXXXX

        sizex: 13
        sizey: 13
        len: 169
        187 251 238 190 187 0 0 0 0 90 13 0 0 0 13 0 0 0 70 2 0 0 0 0 0 0 0 2
        2 0 0 0 1 0 0 0 3 10 0 0 0 11 0 0 0 0 0 0 0 0 0 0 0 0 255 252 0 127
        251 16 90 238 213 70 235 180 5 174 173 69 107 255 0 31 255 128
        len: 76
        43 50 21 60 32 26 2 33 13 59 33 13 59 32 6 2 33 2 60 59 33 2 33 2 59 1
        59 33 3 33 10 59 33 11 60 60 60 31 32 28 7 0 32 31 3 29 4 32 26 2 26 32
        21 6 32 6 2 25 32 20 5 3 18 32 13 5 11 32 11 3 31 0 33 31 31 32 0 4
        len: 78
        hoLyWQ2XDxXDxW62X2yxX2X2x1xX3XAxXByyyVWS70WV3T4WQ2QWL6W62PWK53IWD5BWB3V
        0XVVW04
        len: 78
        43 50 21 60 32 26 2 33 13 59 33 13 59 32 6 2 33 2 60 59 33 2 33 2 59 1
        59 33 3 33 10 59 33 11 60 60 60 31 32 28 7 0 32 31 3 29 4 32 26 2 26 
        32 21 6 32 6 2 25 32 20 5 3 18 32 13 5 11 32 11 3 31 0 33 31 31 32 0 4
        len: 78
        187 251 238 190 187 0 0 0 0 90 13 0 0 0 13 0 0 0 70 2 0 0 0 0 0 0 0 2 
        2 0 0 0 1 0 0 0 3 10 0 0 0 11 0 0 0 0 0 0 0 0 0 0 0 0 255 252 0 127 251
        16 90 238 213 70 235 180 5 174 173 69 107 255 0 31 255 128
        len: 76
        true


The 2nd layer

        187 251 238 190 187 0 0 0 0     # ID frame (0.0.0 0)
        90 13 0 0 0 13 0 0 0            # SIZE frame (sizex 13, sizey 13)
        70 2 0 0 0 0 0 0 0              # FLAG frame (aflag)
        2 2 0 0 0 1 0 0 0               # START frame
        3 10 0 0 0 11 0 0 0             # GOAL frame
        0 0 0 0 0 0 0 0 0               # END frame
        255 252 0 127 251               # Data region
        16 90 238 213 70
        235 180 5 174 173
        69 107 255 0 31
        255 128

The 3rd layer

        43 [187 251] 50 [238 190] 21 [187] 60 [0 0 0 0]
        32 26 2 [90] 33 13 [13] 59 [0 0 0] 33 13 [13] 59 [0 0 0]
        32 6 2 [70] 33 2 [2] 60 [0 0 0 0] 59 [0 0 0]
        33 2 [2] 33 2 [2] 59 [0 0 0] 1 [1] 59 [0 0 0]
        33 3 [3] 33 10 [10] 59 [0 0 0] 33 11 [11] 60 [0 0 0 0]
        60 [0 0 0 0] 60 [0 0 0 0]
        31 [255] 32 28 7 [252] 0 [0] 32 31 3 [127] 29 [251]
        4 [16] 32 26 2 [90] 26 [238] 32 21 6 [213] 32 6 2 [70]
        25 [235] 32 20 5 [180] 3 [5] 18 [174] 32 13 5 [173]
        11 [69] 32 11 3 [107] 31 [255] 0 [0] 33 31 [31]
        31 [255] 32 0 4 [128]

The 4th layer

        hoLy
        WQ2XDxXDx
        W62X2yx
        X2X2x1x
        X3XAxXBy
        yy
        VWS70WV3T
        4WQ2QWL6W62
        PWK53IWD5
        BWB3V0XV
        VW04

## Author
itchyny <https://github.com/itchyny>

