//! @file
//! @brief The software_timer source file.


/*---------------------------------------------------------------------*
 *  private: include files
 *---------------------------------------------------------------------*/

#include "software_timer.h"

#include <math.h>


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
/*---------------------------------------------------------------------*
 *  public:  functions
 *---------------------------------------------------------------------*/

bool software_timer_elapsed (software_timer_t *object)
{
    software_timer_timer_info_t * timer_info = object->timer_info;
    volatile uint64_t * overflows_ptr = timer_info->overflows;
    volatile uint16_t * counter_ptr = timer_info->counter;

#if true

    // The next two lines are not thread/interrupt safe. It is therefore
    // important that the variable `overflows` is always read in first.
    // This means that in an invent of an overflow the value read is
    // intentionally too small and the timer is not marked as expired early.
    // But this if-branch is faster than the else-branch.

    uint64_t overflows = *overflows_ptr;
    uint16_t counter = *counter_ptr;

#else

    // The `overflows` and `counter` read operations are not thread/interrupt safe.
    // By reading in twice, it is possible to check whether there was
    // an overflow and, if so, to read in the correct value.

    uint16_t counter_a = *counter_ptr;
    uint64_t overflows = *overflows_ptr;
    uint16_t counter = *counter_ptr;

    if(counter < counter_a)
    {
        overflows = *overflows_ptr;
    }

#endif

    uint64_t end_overflows = object->end_overflows;
    uint32_t end_counter = object->end_counter;

    if(((counter >= end_counter) && (overflows == end_overflows)) || (overflows > end_overflows))
    {
        uint16_t duration_counter = object->duration_counter;
        uint64_t duration_overflows = object->duration_overflows;

        end_counter += duration_counter;
        end_overflows += duration_overflows;

        uint32_t capture_compare = UINT32_C(1) + timer_info->capture_compare;

        if( capture_compare <= end_counter )
        {
            end_overflows += 1;
            end_counter -= capture_compare;
        }

        // ---- ---- ---- ----

        object->end_overflows = end_overflows;
        object->end_counter = (uint16_t)end_counter;

        if(NULL != object->tick) { object->tick(object); }

        return true;
    }
    else
    {
        return false;
    }
}

bool software_timer_elapsed_prevent_multiple_triggers (software_timer_t *object)
{
    software_timer_timer_info_t * timer_info = object->timer_info;
    volatile uint64_t * overflows_ptr = timer_info->overflows;
    volatile uint16_t * counter_ptr = timer_info->counter;

#if true

    uint64_t overflows = *overflows_ptr;
    uint16_t counter = *counter_ptr;

#else

    uint16_t counter_a = *counter_ptr;
    uint64_t overflows = *overflows_ptr;
    uint16_t counter = *counter_ptr;

    if(counter < counter_a)
    {
        overflows = *overflows_ptr;
    }

#endif

    uint64_t end_overflows = object->end_overflows;
    uint32_t end_counter = object->end_counter;

    if(((counter >= end_counter) && (overflows == end_overflows)) || (overflows > end_overflows))
    {
        uint16_t duration_counter = object->duration_counter;
        uint64_t duration_overflows = object->duration_overflows;

        end_counter += duration_counter;
        end_overflows += duration_overflows;

        uint32_t capture_compare = UINT32_C(1) + timer_info->capture_compare;

        if( capture_compare <= end_counter )
        {
            end_overflows += 1;
            end_counter -= capture_compare;
        }

        // ---- ---- ---- ----

        if(((counter >= end_counter) && (overflows == end_overflows)) || (overflows > end_overflows))
        {
            double duration_current = ((double)capture_compare) * (double)(overflows - end_overflows) + ((double)counter - (double)end_counter);
            double duration = (double)capture_compare * (double)duration_overflows + duration_counter;

            if(0 != duration)
            {
                double duration_target = ceil(duration_current * object->ticks_per_second) * duration;
                uint64_t duration_target_per_CC = (uint64_t)(duration_target * timer_info->capture_compare_inverse);

                end_overflows = duration_target_per_CC + end_overflows;
                end_counter = (uint16_t)(duration_target - ((double)duration_target_per_CC * (double)capture_compare)) + end_counter;

                if( capture_compare <= end_counter )
                {
                    end_overflows += 1;
                    end_counter -= capture_compare;
                }
            }
        }

        // ---- ---- ---- ----

        object->end_overflows = end_overflows;
        object->end_counter = (uint16_t)end_counter;

        if(NULL != object->tick) { object->tick(object); }

        return true;
    }
    else
    {
        return false;
    }
}

double software_timer_get_time (software_timer_timestamp_t * timestamp)
{
    software_timer_timer_info_t * timer_info = timestamp->timer_info;

    double seconds_per_tick = timer_info->seconds_per_tick;
    double counter = seconds_per_tick * timestamp->counter;

    // @info: The first variable must be cast because the calculation can exceed 64 bits,
    // the second cast removes the warning that accuracy could be lost with the cast
    double overflow = ( (double)seconds_per_tick * (double)timestamp->overflows * ( (uint32_t)timer_info->capture_compare + 1));

    return overflow + counter;
}

void software_timer_get_timespec (struct timespec * result_timespec, software_timer_timestamp_t * timestamp)
{
    double time = software_timer_get_time(timestamp);

#if false

    // It is unrealistic to assume that the time value will ever be so high

    if( UINT64_MAX < time )
    {
        result_timespec->tv_sec = (time_t)(UINT64_MAX);
        result_timespec->tv_nsec = (long)UINT32_C(999999999);
        return;
    }

#endif

    uint64_t seconds = (uint64_t)time;

    // @info: It could lose precision with the cast
    time = time - (double)seconds;

    result_timespec->tv_sec = (time_t)seconds;
    result_timespec->tv_nsec = (long)(time * 1.0e09);

#if false

    // This additional effort can increase the accuracy,
    // but the effort is disproportionate to the benefit.

    software_timer_t timer_1 = SOFTWARE_TIMER_INIT_HALT(timestamp->timer_info);
    software_timer_set_duration(&timer_1,  seconds);

    int32_t counter = timestamp->counter;
    uint64_t overflows = timestamp->overflows;

    counter -= timer_1.duration_counter;
    if( 0 > counter)
    {
        --overflows;
    }

    overflows -= timer_1.duration_overflows;


    timestamp->counter = counter;
    timestamp->overflows = overflows;

    time = software_timer_get_time(timestamp);
    double tmp = time * 1.0e09;

    result_timespec->tv_nsec = (long)(tmp);

#endif
}

void software_timer_get_timestamp (software_timer_t * object, software_timer_timestamp_t * timestamp)
{
    software_timer_timer_info_t * timer_info = object->timer_info;
    volatile uint64_t * overflows_ptr = timer_info->overflows;
    volatile uint16_t * counter_ptr = timer_info->counter;

    // The `overflows` and `counter` read operations are not thread/interrupt safe.
    // By reading in twice, it is possible to check whether there was
    // an overflow and, if so, to read in the correct value.
    uint16_t counter_a = *counter_ptr;
    uint64_t overflows = *overflows_ptr;
    uint16_t counter = *counter_ptr;
    if(counter < counter_a)
    {
        overflows = *overflows_ptr;
    }

    timestamp->counter = counter;
    timestamp->overflows = overflows;
    timestamp->timer_info = timer_info;
}

void software_timer_init_halt (software_timer_t * object, software_timer_timer_info_t * timer_info)
{
    object->end_counter = 0;
    object->end_overflows = UINT64_MAX;
    object->duration_counter = 0;
    object->duration_overflows = 0;
    object->time_in_seconds = 0;
    object->ticks_per_second = 0;
    object->timer_info = timer_info;
    object->tick = 0;
}

bool software_timer_is_running (software_timer_t * object)
{
    return UINT64_MAX != object->end_overflows;
}

bool software_timer_is_stopped (software_timer_t * object)
{
    return UINT64_MAX == object->end_overflows;
}

software_timer_duration_flag_t software_timer_set_duration (software_timer_t * object, double time_in_seconds)
{
    uint16_t duration_counter;
    uint64_t duration_overflows;

    software_timer_timer_info_t * timer_info = object->timer_info;
    uint64_t ticks_per_second = timer_info->ticks_per_second;

    software_timer_duration_flag_t state = SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS;

    object->time_in_seconds = time_in_seconds;

    object->ticks_per_second = 1.0 / time_in_seconds;

    // @info: The first variable must be cast because the calculation can exceed 64 bits,
    // the second and third cast removes the warning that accuracy could be lost with the cast
    double duration_overflows_dbl = ( (double)time_in_seconds * (double)timer_info->capture_compare_inverse ) *  (double)ticks_per_second;

    if( UINT64_MAX < duration_overflows_dbl )
    {
        state = (software_timer_duration_flag_t) (state | SOFTWARE_TIMER_DURATION_FLAG_GREATER_MAX);
        duration_overflows = UINT64_MAX;
        duration_counter = UINT16_MAX;
    }
    else
    {
        duration_overflows = (uint64_t)duration_overflows_dbl;

        // @info: The first variable must be cast because the calculation can exceed 64 bits,
        // the second and third cast removes the warning that accuracy could be lost with the cast
        double duration_counter_dbl =
            ( (double)time_in_seconds * (double)ticks_per_second ) -
            ( (double)duration_overflows * (( (uint32_t)timer_info->capture_compare) + 1));

        duration_counter = (uint16_t)duration_counter_dbl;

        if(0 == duration_counter && 0 == duration_overflows)
        {
            state = (software_timer_duration_flag_t) (state | SOFTWARE_TIMER_DURATION_FLAG_SMALLER_ONE);
        }

        if(duration_counter_dbl > duration_counter)
        {
            ++duration_counter;
            state = (software_timer_duration_flag_t) (state | SOFTWARE_TIMER_DURATION_FLAG_NO_INTEGER);
        }
    }

    object->duration_overflows = duration_overflows;
    object->duration_counter = duration_counter;

    return (software_timer_duration_flag_t)state;
}

void software_timer_start (software_timer_t *object)
{
    software_timer_timer_info_t * timer_info = object->timer_info;
    volatile uint64_t * overflows_ptr = timer_info->overflows;
    volatile uint16_t * counter_ptr = timer_info->counter;

    // The `overflows` and `counter` read operations are not thread/interrupt safe.
    // By reading in twice, it is possible to check whether there was
    // an overflow and, if so, to read in the correct value.
    uint16_t counter_a = *counter_ptr;
    uint64_t overflows = *overflows_ptr;
    uint16_t counter = *counter_ptr;
    if(counter < counter_a)
    {
        overflows = *overflows_ptr;
    }

    uint32_t end_counter = (uint32_t)counter + object->duration_counter;
    overflows += object->duration_overflows;

    uint32_t capture_compare = UINT32_C(1) + timer_info->capture_compare;

    if( capture_compare <= end_counter )
    {
        overflows += 1;
        end_counter -= capture_compare;
    }

    object->end_overflows = overflows;
    object->end_counter = (uint16_t)end_counter;
}

void software_timer_stop (software_timer_t *object)
{
    object->end_overflows = UINT64_MAX;
}

void software_timer_sub_timestamp (software_timer_timestamp_t * result_and_minuend, software_timer_timestamp_t * subtrahend)
{
    int32_t counter = result_and_minuend->counter;
    uint64_t overflows = result_and_minuend->overflows;

    counter -= subtrahend->counter;
    if( 0 > counter)
    {
        --overflows;
    }

    overflows -= subtrahend->overflows;

    result_and_minuend->counter = (uint16_t)counter;
    result_and_minuend->overflows = overflows;
}


/*---------------------------------------------------------------------*
 *  eof
 *---------------------------------------------------------------------*/
