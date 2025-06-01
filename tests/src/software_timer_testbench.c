/*---------------------------------------------------------------------*
 *  private: include files
 *---------------------------------------------------------------------*/

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <assert.h>

#include "software_timer.h"


/*---------------------------------------------------------------------*
 *  private: definitions
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 *  private: typedefs
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 *  private: variables
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 *  public:  variables
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 *  private: function prototypes
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 *  private: functions
 *---------------------------------------------------------------------*/

#define TIMER_BASE_INIT(COUNTER, CAPTURE_COMPARE, OVERFLOWS, OVERFLOW_EVENT) \
{                      \
    (COUNTER),         \
    (CAPTURE_COMPARE), \
    (OVERFLOWS),       \
    (OVERFLOW_EVENT    \
}                    /*;*/

typedef struct hardware_timer_s hardware_timer_t;

typedef void (*hardware_timer_handler_t)(hardware_timer_t * object);

//! Hardware timer
typedef struct hardware_timer_s
{
    //! @brief Counter
    uint16_t counter;

    //! Timer will hold the value for one cycle and next time its set to 0, So the value 0x0F will need 16 cycles
    uint16_t capture_compare;

    //! @brief Capture Compare
    uint64_t overflows;

    //! @brief Overflow event
    hardware_timer_handler_t overflow_event;
}hardware_timer_t;

void hardware_timer_set(hardware_timer_t * timer, uint16_t counter);
void hardware_timer_increment(hardware_timer_t * timer);
double hardware_timer_no(hardware_timer_t * timer);

void hardware_timer_test(void);

void hardware_timer_set(hardware_timer_t * timer, uint16_t counter)
{
    timer->counter = counter;
}

void hardware_timer_increment(hardware_timer_t * timer)
{
    uint32_t counter = timer->counter + 1;
    if( timer->capture_compare < counter )
    {
        counter = 0;
        timer->overflows++;
        if(NULL != timer->overflow_event){ timer->overflow_event(timer); }
    }
    timer->counter = (uint16_t)counter;
}

double hardware_timer_no(hardware_timer_t * timer)
{
    return (double)timer->overflows * ((uint32_t)timer->capture_compare + 1) + timer->counter;
}

void hardware_timer_overflows_handler(hardware_timer_t * object)
{
    if(NULL == object){ return; }
}


void print_timer_info(software_timer_t * object)
{
    double cnt =
        (double)*(object->timer_info->overflows) *
        ( ( (uint32_t)object->timer_info->capture_compare) + 1) +
        *(object->timer_info->counter);

    printf("counter: %5" PRIu16 ", overflows: %5" PRIu64 ", No: %5f\n",
        *(object->timer_info->counter),
        *(object->timer_info->overflows),
        cnt
    );

    fflush(stdout);
}

void print_function_info(const char name[])
{
    printf("%s\n", name);
    fflush(stdout);
}


#if true // test software_timer_get_time

void software_timer_test_get_timestamp()
{
    print_function_info(__func__);

    software_timer_timestamp_t timestamp;
    memset(&timestamp, 0, sizeof(timestamp) );

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 0,
        .prescaler = 0,
        .ticks_per_second = 0,
        .capture_compare_inverse = 0.0
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);
    timer_1.on_tick = print_timer_info;

    hw_timer_1.counter = 10;
    hw_timer_1.overflows = 100;
    software_timer_get_timestamp(&timer_1, &timestamp);
    assert( 10 == timestamp.counter );
    assert( 100 == timestamp.overflows );
    assert( &sw_timer_1 == timestamp.timer_info );

    memset(&timestamp, 0, sizeof(timestamp) );

    hw_timer_1.counter = 1;
    hw_timer_1.overflows = 3;
    software_timer_get_timestamp(&timer_1, &timestamp);
    assert( 1 == timestamp.counter );
    assert( 3 == timestamp.overflows );
    assert( &sw_timer_1 == timestamp.timer_info );
    memset(&timestamp, 0, sizeof(timestamp) );

    hw_timer_1.counter = UINT16_MAX;
    hw_timer_1.overflows = UINT64_MAX;
    software_timer_get_timestamp(&timer_1, &timestamp);
    assert( UINT16_MAX == timestamp.counter );
    assert( UINT64_MAX == timestamp.overflows );
    assert( &sw_timer_1 == timestamp.timer_info );
    memset(&timestamp, 0, sizeof(timestamp) );
}

#endif

#if true // test software_timer_calculate_and_set_duration

void software_timer_test_set_duration_16bit()
{
    print_function_info(__func__);

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = NULL,
        .overflows = NULL,
        .capture_compare = 65535,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);
    timer_1.on_tick = print_timer_info;

    software_timer_duration_flag_t flag;

    flag = software_timer_calculate_and_set_duration(&timer_1,  10.0e-9);
    assert( ( SOFTWARE_TIMER_DURATION_FLAG_SMALLER_ONE | SOFTWARE_TIMER_DURATION_FLAG_NO_INTEGER ) == flag );
    assert( 1 == timer_1.duration_counter );
    assert( 0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  23.529411764705882352941176470588e-9);
    assert( ( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS ) == flag );
    assert( 1 == timer_1.duration_counter );
    assert( 0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  40.0e-9);
    assert( SOFTWARE_TIMER_DURATION_FLAG_NO_INTEGER == flag );
    assert( 2 == timer_1.duration_counter );
    assert( 0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  47.058823529411764705882352941176e-9);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 2 == timer_1.duration_counter );
    assert( 0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1, 400.0e-9);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 17 == timer_1.duration_counter );
    assert( 0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,   4.0e-6);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 170 == timer_1.duration_counter );
    assert( 0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,   1.0e-3);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 42500 == timer_1.duration_counter );
    assert(     0 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,   4.0e-3);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 38928 == timer_1.duration_counter );
    assert(     2 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  10.0e-3);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 31784 == timer_1.duration_counter );
    assert(     6 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1, 500.0e-3);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 16336 == timer_1.duration_counter );
    assert(   324 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,   1.0e-0); // 1 sec
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 32672 == timer_1.duration_counter );
    assert(   648 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  30.0e-0); // 30 sec
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 62656 == timer_1.duration_counter );
    assert( 19454 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  60.0e-0); // 1 min
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 59776 == timer_1.duration_counter );
    assert( 38909 == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  1800.0e-0); // 30 min
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 23808 == timer_1.duration_counter );
    assert( UINT64_C(1167297) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  3600.0e-0); // 1h
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 47616 == timer_1.duration_counter );
    assert( UINT64_C(2334594) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  86400.0e-0); // 24 h
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 28672 == timer_1.duration_counter );
    assert( UINT64_C(56030273) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  108000.0e-0); // 30 h
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 52224 == timer_1.duration_counter );
    assert( UINT64_C(70037841) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  2592000.0e-0); // 30 days
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 8192 == timer_1.duration_counter );
    assert( UINT64_C(1680908203) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  31536000.0e-0); // 365 days
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 45056 == timer_1.duration_counter );
    assert( UINT64_C(20451049804) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1,  28.5E15 ); // 903,729,071 years, 196 days, 2 hours, 40 minutes, 0 seconds
    assert( SOFTWARE_TIMER_DURATION_FLAG_GREATER_MAX == flag );
    assert( UINT16_MAX == timer_1.duration_counter );
    assert( UINT64_MAX == timer_1.duration_overflows );
}

void software_timer_test_set_duration_4bit()
{
    print_function_info(__func__);

    software_timer_duration_flag_t flag;

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = NULL,
        .overflows = NULL,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .capture_compare_inverse = 1.0 / (15.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);
    timer_1.on_tick = print_timer_info;

    flag = software_timer_calculate_and_set_duration(&timer_1, 117.64705882352941176470588235294e-9);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 5 == timer_1.duration_counter );
    assert( UINT64_C(0) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1, 117.647e-9);
    assert( SOFTWARE_TIMER_DURATION_FLAG_NO_INTEGER == flag );
    assert( 5 == timer_1.duration_counter );
    assert( UINT64_C(0) == timer_1.duration_overflows );

    flag = software_timer_calculate_and_set_duration(&timer_1, 1.0);
    assert( SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS == flag );
    assert( 0 == timer_1.duration_counter );
    assert( UINT64_C(2656250) == timer_1.duration_overflows );
}

void software_timer_test_set_duration_inline()
{
    print_function_info(__func__);

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = NULL,
        .overflows = NULL,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .capture_compare_inverse = 1.0 / (15.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);
    timer_1.on_tick = print_timer_info;

    software_timer_duration_t duration =
    {
        31536000.0e-0,
        31.709791983764586504312531709792e-9,
        45056,
        UINT64_C(20451049804)
    }; // 365 days

    software_timer_set_duration(&timer_1, &duration);
    assert( 45056 == timer_1.duration_counter );
    assert( UINT64_C(20451049804) == timer_1.duration_overflows );
}

#endif


void software_timer_test_get_time()
{
    print_function_info(__func__);

    software_timer_timestamp_t timestamp;

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 65535,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .seconds_per_tick = 1.0 / 42500000.0,
        .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);

    hw_timer_1.counter = 5;
    hw_timer_1.overflows = 1;

    software_timer_get_timestamp(&timer_1, &timestamp);

    double time = software_timer_get_time(&timestamp);

    software_timer_calculate_and_set_duration(&timer_1,  23.529411764705882352941176470588e-9);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    time = software_timer_get_time(&timestamp);
    assert( 23.529411764705882352941176470588e-9 == time );

    software_timer_calculate_and_set_duration(&timer_1,  40.0e-9);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    time = software_timer_get_time(&timestamp);
    assert( 47.058823529411764705882352941176e-9 == time );

    software_timer_calculate_and_set_duration(&timer_1,  400.0e-9);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    time = software_timer_get_time(&timestamp);
    assert( 400.0e-9 == time );
}

void software_timer_test_get_timespec()
{
    struct timespec timspec_value;

    print_function_info(__func__);

    software_timer_timestamp_t timestamp;

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 65535,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .seconds_per_tick = 1.0 / 42500000.0,
        .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);

    software_timer_calculate_and_set_duration(&timer_1,  23.529411764705882352941176470588e-9);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    software_timer_get_timespec(&timestamp, &timspec_value);
    assert(  0 == timspec_value.tv_sec );
    assert( 23 == timspec_value.tv_nsec );


    software_timer_calculate_and_set_duration(&timer_1,  400.0e-9);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    software_timer_get_timespec(&timestamp, &timspec_value);
    assert(   0 == timspec_value.tv_sec );
    assert( 400 == timspec_value.tv_nsec );


    // Lose in significance
    software_timer_calculate_and_set_duration(&timer_1,  9876543210.1234567898);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    software_timer_get_timespec(&timestamp, &timspec_value);
    assert( 9876543210 == timspec_value.tv_sec );
    assert(  123456954 == timspec_value.tv_nsec );

    // Time is round up
    software_timer_calculate_and_set_duration(&timer_1,  9876543210.999999999999);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    software_timer_get_timespec(&timestamp, &timspec_value);
    assert( 9876543211 == timspec_value.tv_sec );
    assert(       1907 == timspec_value.tv_nsec );


    // Time is round up
    software_timer_calculate_and_set_duration(&timer_1,  0.1234567898);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp);
    software_timer_get_timespec(&timestamp, &timspec_value);
    assert(                  0  == timspec_value.tv_sec );
    assert( UINT64_C(123456799) == timspec_value.tv_nsec );

}

void software_timer_test_get_timespec_max()
{
    struct timespec timspec_value;

    print_function_info(__func__);

    software_timer_timestamp_t timestamp;

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 65535,
        .prescaler = 32768,
        .ticks_per_second = 5187,
        .seconds_per_tick = 1.9275294117647058823529411764706e-4,
        .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);

    hw_timer_1.counter = UINT16_MAX;
    hw_timer_1.overflows = UINT64_MAX;

    software_timer_get_timestamp(&timer_1, &timestamp);
    software_timer_get_timespec(&timestamp, &timspec_value);
    assert(                  0  == timspec_value.tv_sec );
    assert( UINT64_C(123456799) == timspec_value.tv_nsec );

}

void software_timer_test_sub_timestamp()
{
    print_function_info(__func__);

    software_timer_timestamp_t result_and_minuend;
    software_timer_timestamp_t timestamp_subtrahend;

    result_and_minuend.counter = 10;
    result_and_minuend.overflows = 20;

    timestamp_subtrahend.counter = 5;
    timestamp_subtrahend.overflows = 5;

    software_timer_sub_timestamp(&result_and_minuend, &timestamp_subtrahend);
    assert( 5 == result_and_minuend.counter );
    assert( 15 == result_and_minuend.overflows );


    result_and_minuend.counter = 10;
    result_and_minuend.overflows = 20;

    timestamp_subtrahend.counter = 20;
    timestamp_subtrahend.overflows = 5;

    software_timer_sub_timestamp(&result_and_minuend, &timestamp_subtrahend);
    assert( ((uint16_t)-10) == result_and_minuend.counter );
    assert( 14 == result_and_minuend.overflows );


    result_and_minuend.counter = 0;
    result_and_minuend.overflows = 20;

    timestamp_subtrahend.counter = 0;
    timestamp_subtrahend.overflows = 25;

    software_timer_sub_timestamp(&result_and_minuend, &timestamp_subtrahend);
    assert( 0 == result_and_minuend.counter );
    assert( ((uint64_t)UINT64_C(-5)) == result_and_minuend.overflows );


    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 65535,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .seconds_per_tick = 1.0 / 42500000.0,
        .capture_compare_inverse = 1.0 / (65535.0 + 1.0),
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);

    software_timer_calculate_and_set_duration(&timer_1, 1000.0e-3);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &result_and_minuend);

    software_timer_calculate_and_set_duration(&timer_1,  500.0e-3);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp_subtrahend);

    software_timer_sub_timestamp(&result_and_minuend, &timestamp_subtrahend);

    double time = software_timer_get_time(&result_and_minuend);
    assert( 500.0e-3 == time );


    software_timer_calculate_and_set_duration(&timer_1, 333.0e-3);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &result_and_minuend);

    software_timer_calculate_and_set_duration(&timer_1,  223.0e-3);
    hw_timer_1.counter = timer_1.duration_counter;
    hw_timer_1.overflows = timer_1.duration_overflows;
    software_timer_get_timestamp(&timer_1, &timestamp_subtrahend);

    software_timer_sub_timestamp(&result_and_minuend, &timestamp_subtrahend);

    time = software_timer_get_time(&result_and_minuend);
    assert( 110.0e-3 == time || 0.10999999999999999 == time);
}

#if true // test others

void software_timer_test_init_halt()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(&sw_timer_1);
    timer_1.duration_counter = 4;
    timer_1.duration_overflows = 0;
    timer_1.end_counter = 2;
    timer_1.on_tick = print_timer_info;

    bool ticked;

    for(uint64_t overflows = 0; overflows <= 20; overflows++)
    {
        for(uint16_t counter = 0; counter <= 15; counter++)
        {
            assert( counter == hw_timer_1.counter && overflows == hw_timer_1.overflows);
            ticked = software_timer_elapsed(&timer_1);
            assert( false == ticked );

            hardware_timer_increment(&hw_timer_1);
        }
    }
}

void software_timer_duration0()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 0,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    for(uint64_t overflows = 0; overflows <= 20; overflows++)
    {
        for(uint16_t counter = 0; counter <= 15; counter++)
        {
            assert( counter == hw_timer_1.counter && overflows == hw_timer_1.overflows);
            ticked = software_timer_elapsed(&timer_1);
            assert( true == ticked );

            hardware_timer_increment(&hw_timer_1);
        }
    }
}

void software_timer_with_null_tick_handler()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 0,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,
        .on_tick = NULL,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    print_timer_info(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    print_timer_info(&timer_1);
    assert( true == ticked );
}

void software_timer_test_stop()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 0,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

    software_timer_stop(&timer_1);

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(10 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(11 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(12 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert(13 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(14 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(15 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );
}

void software_timer_test_start()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 4,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    timer_1.end_overflows = UINT64_MAX;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    software_timer_start(&timer_1);

    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(10 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert(11 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(12 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(13 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(14 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert(15 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

}

void software_timer_start2()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 4,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    timer_1.end_overflows = UINT64_MAX;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    software_timer_start(&timer_1);

    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(10 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert(11 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(12 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    timer_1.duration_counter = 5;
    software_timer_start(&timer_1);

    hardware_timer_increment(&hw_timer_1);
    assert(13 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(14 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(15 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(10 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(11 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

}

void software_timer_test_match_capture_compare()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 4,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(10 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(11 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(12 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert(13 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(14 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(15 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );
}

void software_timer_test_greater_than_capture_compare()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 0,
        .end_overflows = 0,
        .duration_counter = 5,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(10 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert(11 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(12 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(13 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(14 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert(15 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 1 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 2 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 6 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 7 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 8 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 9 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );
}

void software_timer_late_interrupt()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
    };

    software_timer_t timer_1 =
    {
        .end_counter = 5,
        .end_overflows = 0,
        .duration_counter = 5,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.0,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 1; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 2; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 3; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 4; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 5; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    timer_1.end_counter = 5;
    timer_1.end_overflows = 1;

    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 15; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 0; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 1; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 2; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 3; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 4; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 5; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 6; hw_timer_1.overflows = 0; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 6; hw_timer_1.overflows = 1;
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    timer_1.end_counter = 0;
    timer_1.end_overflows = 2;

    hw_timer_1.counter = 15; hw_timer_1.overflows = 1; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 0; hw_timer_1.overflows = 1; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 1; hw_timer_1.overflows = 1; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 2; hw_timer_1.overflows = 1; // interrupt not called
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 2; hw_timer_1.overflows = 2;
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );
}

void software_timer_slow_checking()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .capture_compare_inverse = 1.0 / ( UINT32_C(1) + 15 )
    };

    software_timer_t timer_1 =
    {
        .end_counter = 5,
        .end_overflows = 0,
        .duration_counter = 5,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.2,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 1; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 2; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 3; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 4; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 5; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );


    hw_timer_1.counter = 2; hw_timer_1.overflows = 1;
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );


    // ---- ---- ----
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );
    // ---- ---- ----


    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( true == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed(&timer_1);
    assert( false == ticked );
}

void software_timer_slow_checking_prevent_multiple_triggers()
{
    print_function_info(__func__);

    hardware_timer_t hw_timer_1 =
    {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = NULL,
    };

    software_timer_timer_info_t sw_timer_1 =
    {
        .counter = &hw_timer_1.counter,
        .overflows = &hw_timer_1.overflows,
        .capture_compare = 15,
        .prescaler = 4,
        .ticks_per_second = 42500000,
        .capture_compare_inverse = 1.0 / ( UINT32_C(1) + 15 )
    };

    software_timer_t timer_1 =
    {
        .end_counter = 5,
        .end_overflows = 0,
        .duration_counter = 5,
        .duration_overflows = 0,

        .time_in_seconds = 0.0,
        .ticks_per_second = 0.2,

        .timer_info = &sw_timer_1,

        .on_tick = print_timer_info,
    };

    bool ticked;

    // hardware_timer_increment(&hw_timer_1);
    assert( 0 == hw_timer_1.counter && 0 == hw_timer_1.overflows);
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 1; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 2; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 3; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 4; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );

    hw_timer_1.counter = 5; hw_timer_1.overflows = 0;
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( true == ticked );


    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );


    hw_timer_1.counter = 2; hw_timer_1.overflows = 1;
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( true == ticked );


    // ---- ---- ----
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );
    // ---- ---- ----


    hardware_timer_increment(&hw_timer_1);
    assert( 3 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 4 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( true == ticked );

    hardware_timer_increment(&hw_timer_1);
    assert( 5 == hw_timer_1.counter && 1 == hw_timer_1.overflows);
    ticked = software_timer_elapsed_prevent_multiple_triggers(&timer_1);
    assert( false == ticked );
}




#endif


void software_timer_run_example_1_ticked(software_timer_t * object)
{
    software_timer_timestamp_t timestamp;
    software_timer.GetTimestamp(object, &timestamp);
    printf("Ticked at: %lf" " seconds\n", software_timer.GetTime(&timestamp) );
    fflush(stdout);
}

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

void software_timer_run_example_1()
{
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
}


/*---------------------------------------------------------------------*
 *  public:  functions
 *---------------------------------------------------------------------*/


void hardware_timer_test(void)
{
    hardware_timer_t timer = {
        .counter = 0,
        .capture_compare = 0x0F,
        .overflows = 0,
        .overflow_event = hardware_timer_overflows_handler,
    };

    printf("counter: %5" PRIu16 ", overflows: %5" PRIu64 ", No: %5f\n",
        timer.counter,
        timer.overflows,
        hardware_timer_no(&timer));

    fflush(stdout);

    for(int i = 0; i < 35; i++)
    {
        hardware_timer_increment(&timer);

        printf("counter: %5" PRIu16 ", overflows: %5" PRIu64 ", No: %5f\n",
            timer.counter,
            timer.overflows,
            hardware_timer_no(&timer));

        fflush(stdout);
    }

    timer.counter = 0;
    timer.overflows = 0;
    timer.capture_compare = 0x05;

    printf("counter: %5" PRIu16 ", overflows: %5" PRIu64 ", No: %5f\n",
        timer.counter,
        timer.overflows,
        hardware_timer_no(&timer));

    fflush(stdout);

    for(int i = 0; i < 35; i++)
    {
        hardware_timer_increment(&timer);

        printf("counter: %5" PRIu16 ", overflows: %5" PRIu64 ", No: %5f\n",
            timer.counter,
            timer.overflows,
            hardware_timer_no(&timer));

        fflush(stdout);
    }
}

bool software_timer_test(void)
{
    // test software_timer_get_timestamp
    software_timer_test_get_timestamp();

    // test software_timer_calculate_and_set_duration
    software_timer_test_set_duration_16bit();
    software_timer_test_set_duration_4bit();
    software_timer_test_set_duration_inline();

    software_timer_test_sub_timestamp();
    software_timer_test_get_time();
    software_timer_test_get_timespec();
#if false
    software_timer_test_get_timespec_max();
#endif
    // test others
    software_timer_test_init_halt();
    software_timer_duration0();
    software_timer_with_null_tick_handler();
    software_timer_test_stop();
    software_timer_test_start();
    software_timer_start2();
    software_timer_test_match_capture_compare();
    software_timer_test_greater_than_capture_compare();
    software_timer_late_interrupt();
    software_timer_slow_checking();
    software_timer_slow_checking_prevent_multiple_triggers();

    software_timer_run_example_1();

    return true;
}


/*---------------------------------------------------------------------*
 *  eof
 *---------------------------------------------------------------------*/
