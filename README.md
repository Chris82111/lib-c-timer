# Software Timer

The structures, functions and resulting objects can be used to create any number of 16-bit software timers with only one hardware timer. The timer is designed for single thread applications and enables the realization of a synchronous timer. These timers can use polling to implement time-controlled behavior. Either a branch in the code or a synchronous call of a call-back function using a function handler can be made. To do this, one of the calling function must be executed up cyclically.

## Calling Functions

Various functions are available to implement the logic. Depending on the application, different functions can be used. Methode `Elapsed()` or function `software_timer_elapsed()` guarantees that skipped ticks are made up for:

<picture>
  <source
    media="(prefers-color-scheme: dark)"
    srcset="./docs/elapsed_dark.svg" />
  <img
    alt=""
    src="./docs/elapsed.svg"
    width="250" />
</picture>

With methode `ElapsedPreventMultipleTriggers()` or function `software_timer_elapsed_prevent_multiple_triggers()`, no skipped ticks are made up for:

<picture>
  <source
    media="(prefers-color-scheme: dark)"
    srcset="./docs/elapsed_prevent_multiple_triggers_dark.svg" />
  <img
    alt=""
    src="./docs/elapsed_prevent_multiple_triggers.svg"
    width="250" />
</picture>

Methode `ElapsedOnce()` or function `software_timer_elapsed_once()` waits only once:

<picture>
  <source
    media="(prefers-color-scheme: dark)"
    srcset="./docs/elapsed_once_dark.svg" />
  <img
    alt=""
    src="./docs/elapsed_once.svg"
    width="250" />
</picture>

## Limitation

The limit of the timer is reached when an overflow occurs at the variable `overflows`. This time must be calculated and checked before the library is used. The inline function statement `SOFTWARE_TIMER_MAX_SECONDS()` can be used for this purpose. If the runtime of the system comes close to this time, the library cannot be used safely. For the following example, the time is approx. 901,994,970.9 years.

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
    .capture_compare = 65535, // UINT16_MAX
    .prescaler = 4, // freely selectable
    .ticks_per_second = UINT64_C(42500000), // 170 MHz / 4
    .seconds_per_tick = 1.0 / 42500000.0,
    .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
};

software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&timer_info_1);
```

After initialization, the timer can be used as follows. An interval must be set,
the function `software_timer.CalculateAndSetDuration()` calculates the interval based on
the transferred time in seconds. Optionally, a handler can be set. The end time
of the timer is calculated and set by calling the function
`software_timer.Start()`. To check whether the timer has expired, the function
`software_timer.Elapsed()` must be called cyclically. This returns `false` until
the timer has expired, which leads to the return value `true`. In this case, the
timer restarts automatically, using the last end time and not the current time
of the hardware timer to calculate the next end time. This means that the
interval always remains the same.

```c
double timer_in_seconds = 1.5e-3;
software_timer.CalculateAndSetDuration(&timer_1, timer_in_seconds);

timer_1.on_tick = software_timer_run_example_1_ticked;

if(software_timer.IsStopped(&timer_1))
{
    software_timer.Start(&timer_1);
}

while(1)
{
    if(software_timer.Elapsed(&timer_1))
    {
        software_timer.Stop(&timer_1);

        return; // finish example
    }

    // Example, when the following value is reached, 1.5 ms have elapsed:
    counter = 63750 + 1;
    overflows = 0;

    /* other code */
}
```

A handler can be used to call a function. The function pointer `on_tick` is called
within the function `software_timer.Elapsed()`, therefore this function must be
called cyclically.

```c
void software_timer_run_example_1_ticked(software_timer_t * object)
{
    software_timer_timestamp_t timestamp;
    software_timer.GetTimestamp(object, &timestamp);
    printf("Ticked at: %lf" " seconds\n", software_timer.GetTime(&timestamp) );
    fflush(stdout);
}
```
