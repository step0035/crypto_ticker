# Crypto Ticker

This is a simple ESP32-based crypto ticker that displays the top few cryptocurrencies on a waveshare 2.9 inch e-paper.

## What it can do
* On boot, softAP server will start, connect to the softAP and provision your WiFi credentials to let it connect to your WiFi network (only when booting the first time).
* Retrieve cryptocurrency's details from coingecko per time interval.
* Cycle through the different cryptocurrencies by pressing Button.

## TODO
This is not a finished project, I became lazy and abandoned it. I will pick it up again in the future.
* Use battery, sleep to conserve battery.
* Configure settings like time interval, types of cryptocurrencies, etc, via softAP server.
* 3D print casing.

## Useful stuff
* [waveshare 2.9inch e-paper wiki](https://www.waveshare.com/wiki/2.9inch_e-Paper_Module)
* [image2cpp](https://javl.github.io/image2cpp/)
