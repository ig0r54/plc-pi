# plc-pi
PLC on Raspberry pi

## Установка необходимых пакетов

    sudo apt updtae
    sudo apt install libmodbus5 libmodbus-dev
    sudo apt install libmosquitto-dev
  
## Installing

> The software has been tested on **raspbian**.

Clone the repository:

    git clone https://github.com/ig0r54/plc-pi.git

Go to the repository:

    cd plc-pi

Build the program:

    make

## Running

> Currently the program does not take any arguments. The parameters can be adjusted in the beginning of `main.c`.

Run the program:

    ./plc-pi

