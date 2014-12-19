# maze - a maze generating command
## Installation

    git clone https://github.com/itchyny/maze
    cd ./maze
    autoreconf -i
    ./configure
    make
    sudo make install

## Example

    maze
    maze -w 10 -h 10
    maze -c > test; maze -u `cat test`
    maze -u `maze -c`

## Manual

    man maze

## Author
itchyny <https://github.com/itchyny>

## Repository
https://github.com/itchyny/maze

## License
This software is released under the MIT License, see LICENSE.
