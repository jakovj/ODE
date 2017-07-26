# ODE

Implementation of a serial communication between a computer and a microcontroller. Message (formatted as Sx<CR><LF>) defines
number of diode and potentiometer which are active. The (digital) value  from an active potentiometer is read and then written to
a 4-pack 7-segment display (0-4095, max 4 digits), and then scaled to a 0-100 range. New-scaled value defines how bright will active
diode shine.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes. See deployment for notes on how to deploy the project on a live system.

## Prerequisites

For testing purposes the RS_MSP430F5438A is needed.

## Built With
* IDE

## Tests
```
Test example
```

## Authors

Jakov Jezdić

## License


## Acknowledgments

Hat tip to anyone who's code was used
Inspiration
etc
