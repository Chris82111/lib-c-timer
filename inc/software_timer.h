//! @file

#ifndef INC_SOFTWARE_TIMER_H_
#define INC_SOFTWARE_TIMER_H_

#ifdef __cplusplus
extern "C" {
#endif


/*---------------------------------------------------------------------*
 *  public: include files
 *---------------------------------------------------------------------*/

#include <stdint.h>
#include <stdbool.h>

// Under Unix the structure `timespec` should be present, otherwise see `_TIMESPEC_DEFINED`.
#include <time.h>


/*---------------------------------------------------------------------*
 *  public: define
 *---------------------------------------------------------------------*/

#ifndef _TIMESPEC_DEFINED

//! @brief On non-Unix-based systems, it is not guaranteed that the `timespec` structure is available.
//!
#define _TIMESPEC_DEFINED
struct timespec {
    time_t tv_sec;   /* Seconds */
    long tv_nsec;  /* Nanoseconds */
};
#endif


#ifndef INLINE

//! @brief `inline` macro with compilation-specific attribute, it can be redefined for
//!         a different compiler or platform if required.
//!
#define INLINE inline __attribute__((always_inline))
#endif


#ifndef USE_MULTIPLE_TRIGGERS_MISSED

//! @brief This is necessary if the function has not been called up for a long time and the
//!        timer would have been triggered several times. Prevents the timer from being
//!        triggered several times by calculating the next end_counter time in the raster.
//!        If the function is called continuously, `USE_MULTIPLE_TRIGGERS_MISSED` can be set
//!        to false. The constant variable `use_multiple_trigger_missed` can be read at runtime.
//!
#define USE_MULTIPLE_TRIGGERS_MISSED false
#endif


//! @brief This macro initializes the software timer and stops the timer.
//!
#define SOFTWARE_TIMER_INIT_HALT(TIMER_INFO_ADDRESS) \
{                                                    \
    /* .end_counter        */ (0),                   \
    /* .end_overflows      */ (UINT64_MAX),          \
    /* .duration_counter   */ (0),                   \
    /* .duration_overflows */ (0),                   \
    /* .time_in_seconds    */ (0),                   \
    /* .ticks_per_second   */ (0),                   \
    /* .timer_info         */ (TIMER_INFO_ADDRESS),  \
    /* .tick               */ (0)                    \
}                                                  /*;*/


/*---------------------------------------------------------------------*
 *  public: typedefs
 *---------------------------------------------------------------------*/

//! @brief Return value of the function `software_timer_set_duration()`
//!
typedef enum
{
    //! The calculated number matches the selected time
    SOFTWARE_TIMER_DURATION_FLAG_DURATION_FITS = 0x00,

    //! The calculated number is smaller than 0
    SOFTWARE_TIMER_DURATION_FLAG_SMALLER_ONE   = 0x01,

    //! The calculated number is not a whole number and has been rounded up
    SOFTWARE_TIMER_DURATION_FLAG_NO_INTEGER    = 0x02,

    //! The calculated number is greater than the maximum (`UINT64_MAX`)
    SOFTWARE_TIMER_DURATION_FLAG_GREATER_MAX   = 0x04,
}software_timer_duration_flag_t;

//! @brief The object data of the hardware timer
//!
typedef struct
{
    //! Address of hardware counter
    volatile uint16_t * counter;

    //! Address of overflow counter, external incremented in the hardware
    //! timer interrupt
    volatile uint64_t * overflows;

    //! Capture compare value of the timer. The value must be selected
    //! so that the following statement is true Timer will hold this
    //! value for one cycle and next cycle it's set to 0, so the value
    //! `0x0F` will need 16 cycles (4 bit). If it is an overflow interrupt
    //! timer, e.g. a 16-bit timer, `UINT16_MAX` must be selected.
    //! A value corresponding to the conditions of the timer must be used.
    uint16_t capture_compare;

    //! Prescaler of the timer based on CPU core clock
    uint16_t prescaler;

    //! The tick frequency in Hz of the timer. The frequency in Hz of
    //! the microcontroller is calculated by multiplying this value and
    //! the `prescaler`
    uint64_t ticks_per_second;

    //! This is the inverse value of `ticks_per_second`
    double seconds_per_tick;

    //! This is the inverse value of (`software_timer_timer_info_t.capture_compare` + 1),
    //! necessary for `MULTIPLE_TRIGGERS_MISSED`
    double capture_compare_inverse;

} software_timer_timer_info_t;

//! @brief Timestamp for a specific point in time
//!
typedef struct
{
    //! The automatically incremented hardware counter value
    uint16_t counter;

    //! The interrupt increased counter overflow value
    uint64_t overflows;

    //! Pointer to the timer to which the values refer
    software_timer_timer_info_t * timer_info;

} software_timer_timestamp_t;

//! @brief Prototype of `software_timer_t`, for information see
//!        `software_timer_s`
//!
typedef struct software_timer_s software_timer_t;

//! @brief Function pointer as a handler that is called after the timer has
//!        expired, see `software_timer_t.tick`
//! @param[in,out] object The soft timer object
//!
typedef void (*software_timer_handler_t)(software_timer_t * object);

//! @brief The object data of the software timer
//!
typedef struct software_timer_s
{
    //! Counter which must be reached in combination with
    //! `software_timer_t.end_overflows` for the timer to expire
    uint16_t end_counter;

    //! The overflow of the counter which must be reached in combination
    //! with `software_timer_t.end_counter` for the timer to expire
    uint64_t end_overflows;

    //! The duration of the counter, after the timer expires
    uint16_t duration_counter;

    //! The duration of the overflows, after the timer expires
    uint64_t duration_overflows;

    //! Duration of the timer in seconds
    double time_in_seconds;

    //! Inverse value of `software_timer_t.time_in_seconds`, necessary only for
    //! `MULTIPLE_TRIGGERS_MISSED`
    double ticks_per_second;

    //! Pointer to the timer to which the values refer
    software_timer_timer_info_t * timer_info;

    //! Function that is called after the timer has expired
    software_timer_handler_t tick;

}software_timer_t;


/*---------------------------------------------------------------------*
 *  public: extern variables
 *---------------------------------------------------------------------*/

//! @brief Variable can be read to determine the used preprocessor macro
//!        `USE_MULTIPLE_TRIGGERS_MISSED` at runtime.
//!
extern bool use_multiple_trigger_missed;


/*---------------------------------------------------------------------*
 *  public: function prototypes
 *---------------------------------------------------------------------*/

//! @brief Initializes the timer, normally it is better to use `SOFTWARE_TIMER_INIT_HALT()`
//! @param[out] object The soft timer object
//! @param[in] timer_info Address of the underlying hardware timer
void software_timer_init_halt(software_timer_t * object, software_timer_timer_info_t * timer_info);

//! @brief Checks if the timer is stopped
//! @param[in] object The soft timer object
//! @retval true  when the timer is stopped
//! @retval false if the timer is running
bool software_timer_is_stopped(software_timer_t * object);

//! @brief Checks if the timer is running
//! @param[in] object The soft timer object
//! @retval true  when the timer is running
//! @retval false if the timer is stopped
bool software_timer_is_running(software_timer_t * object);

//! @brief Reads the current values of the underlying timer
//! @param[in,out] object The soft timer object
//! @param[out] timestamp Pointer to the structure in which the values are to be saved
void software_timer_get_timestamp(software_timer_t * object, software_timer_timestamp_t * timestamp);

//! @brief Calculates `result_and_minuend = result_and_minuend - subtrahend`
//! @param[in,out] result_and_minuend Minuend
//! @param[in] subtrahend Subtrahend
void software_timer_sub_timestamp(software_timer_timestamp_t * result_and_minuend, software_timer_timestamp_t * subtrahend);

//! @brief Converts the timestamp value into a seconds value
//! @param[in,out] timestamp Ponter to the timer values
//! @return Returns the time in seconds
double software_timer_get_time(software_timer_timestamp_t * timestamp);

//! @brief Converts the timestamp to the standard structure timespec
//! @param[out] result_timespec Standard `timespec` struct.
//! @param[in] timestamp of the software timer.
void software_timer_get_timespec(struct timespec * result_timespec, software_timer_timestamp_t * timestamp);

//! @brief Sets the duration of the specified timer object according to the specified time
//! @param[in,out] object The soft timer object
//! @param time_in_seconds The time after the timer expires
//! @return Returns the flags with information about the calculated duration
software_timer_duration_flag_t software_timer_set_duration(software_timer_t * object, double time_in_seconds);

//! @brief Starts the timer
//! @param[in,out] object The soft timer object
void software_timer_start(software_timer_t *object);

//! @brief Stops the software timer
//! @param[in,out] object The soft timer object
void software_timer_stop(software_timer_t *object);

//! @brief  Checks if the timer is elapsed
//!         The handler assigned to the function pointer `software_timer_t.tick` is called
//!         If the function is used ensure that the hardware timer interrupt
//!         can be executed, otherwise the `software_timer_timer_info_t.overflow` counter
//!         will not be incremented.
//! @param[in,out] object The soft timer object
//! @retval true  when the timer has elapsed
//! @retval false if the timer has not yet expired
bool software_timer_elapsed(software_timer_t *object);


/*---------------------------------------------------------------------*
 *  public: static inline functions
 *---------------------------------------------------------------------*/

//! @brief To speed up the `software_timer_set_duration()` function, the values can be calculated in advance
//! @param[out] object The soft timer object
//! @param time_in_seconds The time after the timer expires
//! @param ticks_per_second The reciprocal of `software_timer_t.time_in_seconds`
//! @param duration_counter The calculated duration of the counter
//! @param duration_overflows The calculated duration of the overflows
INLINE void SOFTWARE_TIMER_SET_DURATION(software_timer_t * object, double time_in_seconds, double ticks_per_second, uint16_t duration_counter, uint64_t duration_overflows)
{
    object->time_in_seconds = time_in_seconds;
    object->ticks_per_second = ticks_per_second;
    object->duration_counter = duration_counter;
    object->duration_overflows = duration_overflows;
}


/*---------------------------------------------------------------------*
 *  eof
 *---------------------------------------------------------------------*/


#ifdef __cplusplus
}
#endif

#endif /* INC_SOFTWARE_TIMER_H_ */
