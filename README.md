# Software Timer

The structures, functions and resulting objects can be used to create any number
of software timers with only one hardware timer. These timers can use polling to
implement time-controlled behavior.

## Example

In the following example, a timer `timer_1` is created. Any number of timers can
be created, each with their own time intervals. The timer itself contains a
reference to the data of the hardware timer `timer_info_1`, which can be used to
describe the behavior of the hardware timer. Any number of these can also be
created. The 16-bit counter register `counter` usually has a fixed address and
must be looked up in the data sheet. The `overflows` variable must be created by
the user and incremented by one in each hardware timer interrupt. The other
settings are shown here as an example for a 170 MHz microcontroller with a
prescaller of 4 and a 16-bit capture compare register with the maximum value,
i.e. an interval of 2^16 steps. Due to the absolute specifications, 2^16
corresponds to 1.54202... ms.

```c
volatile uint16_t counter = 0; // Example, replace with hardware address
volatile uint64_t overflows = 0;

software_timer_timer_info_t timer_info_1 =
{
    .counter = &counter,
    .overflows = &overflows,
    .capture_compare = 65535,
    .prescaler = 4,
    .ticks_per_second = UINT64_C(42500000),
    .seconds_per_tick = 1.0 / 42500000.0,
    .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
};

software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&timer_info_1);
```

After initialization, the timer can be used as follows. An interval must be set,
the function `software_timer_set_duration()` calculates the interval based on
the transferred time in seconds. Optionally, a handler can be set. The end time
of the timer is calculated and set by calling the function
`software_timer_start()`. To check whether the timer has expired, the function
`software_timer_elapsed()` must be called cyclically. This returns `false` until
the timer has expired, which leads to the return value `true`. In this case, the
timer restarts automatically, using the last end time and not the current time
of the hardware timer to calculate the next end time. This means that the
interval always remains the same.

```c
double timer_in_seconds = 1.5e-3;
software_timer_set_duration(&timer_1, timer_in_seconds);

timer_1.tick = timer_ticked;

if(software_timer_is_stopped(&timer_1))
{
    software_timer_start(&timer_1);
}

while(1)
{
    if(software_timer_elapsed(&timer_1))
    {
        software_timer_stop(&timer_1);
    }

    // Example, when the following value is reached, 1.5 ms have elapsed:
    counter = 63750 + 1;
    overflows = 0;

    /* other code */
}
```

A handler can be used to call a function. The function pointer `tick` is called
within the function `software_timer_elapsed()`, therefore this function must be
called cyclically.

```c
void timer_ticked(software_timer_t * object)
{
    software_timer_timestamp_t timestamp;
    software_timer_get_timestamp(object, &timestamp);
    printf("Ticked at: %lf" " seconds\n", software_timer_get_time(&timestamp) );
    fflush(stdout);
}
```

## Call Test function form Testbench File

The file `software_timer_testbench.c` can be used to test the software timer.
The file is not required to use the software timer and can be deleted. The test
function can be called as follows:

```c
extern "C"
{
    extern void hardware_timer_test(void);
    extern bool software_timer_test(void);
}

int main()
{
    hardware_timer_test();
    software_timer_test();
    return 0;
}
```
