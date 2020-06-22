# GPIO Library

The gpio.h and gpio.c files build the interface and implementation 
of a very rudimentary GPIO library for Raspberry PI Model 3B+.

The library supports the following use-cases:
* Set GPIOs as input or output
* Write to or read from GPIOs
* Measure frequencies on GPIOs
* Create ISRs to react to rising or falling edges on GPIOs

## Macros

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
INP_GPIO(PIN18);
OUT_GPIO(PIN18);
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

## Functions

Partially taken from [3].

### map_peripherals

Maps peripheral memory (start address taken from [2]). Has to be called
with root privileges and before any GPIO interaction happens.

```c
if (map_peripherals() == -1) {
    printf("Fehler beim Mapping des physikalischen GPIO-Registers in den virtuellen Speicherbereich.\n");
    return -1;
}
```

### unmap_peripherals

Unmaps peripherals. No  more GPIO interaction is possible after this call.

```c
unmap_peripherals();
```

### init_isr_func

Create ISR for input flanks.

```c
// trigger isr on rising edges on GPIO 18
init_isr_func(18, EDGE_RISING, isr);

void isr(int pin, int level) {
    if (level == GPIO_ON) {
        // do something
    }
}
```

### del_isr_func

Deactivate ISR and free resources.

```c
// deactivate ISR on GPIO 18
del_isr_func(18);
```

### read_input_freq

Measure input frequency on GPIO in Hz for sampleintervall us

```c
// read frequency on GPIO 18
double freq = read_input_freq(18, DEFAULT_SAMPLE_TIME);
printf("%.2f Hz\n", freq);
```

### get_clock_time

Get current **monotonic** clock time in us.

```c
get_clock_time();
```

## Resources

[1] http://pieter-jan.com/node/15

[2] [BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf](./BCM2837-ARM-Peripherals.-.Revised.-.V2-1.pdf)

[3] https://github.com/joan2937/pigpio/blob/master/pigpio.c