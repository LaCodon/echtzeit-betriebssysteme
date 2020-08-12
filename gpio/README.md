# GPIO Library

The gpio.h and gpio.c files build the interface and implementation 
of a very rudimentary GPIO library for Raspberry PI Model 3B+.

The library supports the following use-cases:
* Set GPIOs as input or output
* Write to or read from GPIOs
* Measure frequencies on GPIOs
* Create ISRs to react to rising or falling edges on GPIOs

# Realtime Library

The realtime.c and realtime.h files contain a few functions which are very useful for creating realtime threads. Those threads are then used in the GPIO Library to make it work in realtime.

# Macros for GPIO handling

The Macros can be used to set the GPIO mode and to read / write to it.
The code is taken from [1].

### INP_GPIO(g)

Set GPIO g as input

```c
// set GPIO 18 as input
INP_GPIO(18);
```

### OUT_GPIO(g)

Set GPIO g as output

```c
// set GPIO 18 as output
// INP and OUT have to be called because bit operators otherwise don't
// override wrong inital state
INP_GPIO(FLOW_SENSOR);
OUT_GPIO(FLOW_SENSOR);
```

### GPIO_SET

Set GPIO high

```c
// set GPIO 18 high
GPIO_SET |= 1 << 18;
```

### GPIO_CLR

Set GPIO low

```c
// set GPIO 18 low
GPIO_CLR |= 1 << 18;
```

### GPIO_READ

Read GPIO value

```c
// read GPIO 18
if (GPIO_READ(18)) {
    // HIGH
} else {
    // LOW
}
```

## Resources

[1] http://pieter-jan.com/node/15

[2] [BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf](./BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)