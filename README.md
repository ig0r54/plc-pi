# plc-pi
PLC on Raspberry pi

## Установка необходимых пакетов

    sudo apt updtae
    sudo apt install libmodbus5 libmodbus-dev
    sudo apt install libmosquitto-dev
  
## Установка

> ПО тестировалось только на **raspbery pi 3 rev B, под raspbian**.

Клонирование директории:

    git clone https://github.com/ig0r54/plc-pi.git

Переходим в папку с проектом:

    cd plc-pi

Построение проекта:

    make

## Запуск

> В данный момент программа запускается как приложение, а не как сервис

Запускаем приложение:

    ./plc-pi

