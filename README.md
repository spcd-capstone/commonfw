Common Firmware
===============

The common firmware used for our all nodes in our home automation project.

To back up factory firmware:

    esptool.py read_flash 0x000 524288 factoryfw.bin

