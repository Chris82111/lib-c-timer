//! @file
//! @brief The software_timer header file.
//!
//! @details The module can be used in C and C++.


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

// Under Unix the structure ::timespec should be present, otherwise see ::HAVE_STRUCT_TIMESPEC , and ::_TIMESPEC_DEFINED.
#include <time.h>


/*---------------------------------------------------------------------*
 *  public: define
 *---------------------------------------------------------------------*/

#ifndef HAVE_STRUCT_TIMESPEC

//! @brief Common in POSIX / Unix
#define HAVE_STRUCT_TIMESPEC

#ifndef _TIMESPEC_DEFINED

//! @brief Common in Windows
#define _TIMESPEC_DEFINED

//! @brief On non-Unix-based systems, it is not guaranteed that the ::timespec structure is available.
struct timespec
{
    //! @brief Seconds
    time_t tv_sec;

    //! @brief Nanoseconds
    long tv_nsec;
};

#endif
#endif


#ifndef INLINE

//! @brief `inline` macro with compilation-specific attribute, it can be redefined for
//! a different compiler or platform if required.
  #if defined(_MSC_VER)
    #define INLINE __forceinline
  #elif defined(__GNUC__) || defined(__clang__)
    #define INLINE inline __attribute__((always_inline))
  #else
    #define INLINE inline
  #endif

#endif


//! @brief This macro initializes the software timer and stops the timer.
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

//! @brief Forward typedef, for information see ::software_timer_s
typedef struct software_timer_s software_timer_t;


//! @brief Function pointer type as a handler that is called after the timer has
//! expired, see ::software_timer_t::tick
//!
//! @param[in,out] object The software timer object
typedef void (*software_timer_handler_t)(software_timer_t * object);


//! @brief Return value of the function ::software_timer_set_duration()
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
typedef struct software_timer_timer_info_s
{
    //! @brief Address of hardware counter
    volatile uint16_t * counter;

    //! @brief Address of overflow counter, external incremented in the hardware timer interrupt
    volatile uint64_t * overflows;

    //! @brief Capture compare value of the timer.
    //! @details The value must be selected
    //! so that the following statement is true Timer will hold this
    //! value for one cycle and next cycle it's set to 0, so the value
    //! `0x0F` will need 16 cycles (4 bit). If it is an overflow interrupt
    //! timer, e.g. a 16-bit timer, `UINT16_MAX` must be selected.
    //! A value corresponding to the conditions of the timer must be used.
    uint16_t capture_compare;

    //! @brief Prescaler of the timer based on CPU core clock
    uint16_t prescaler;

    //! @brief The tick frequency in Hz of the timer.
    //! @details The frequency in Hz of the microcontroller is calculated
    //! by multiplying this value and the ::software_timer_timer_info_s::prescaler
    uint64_t ticks_per_second;

    //! @brief This is the inverse value of ::software_timer_timer_info_s::ticks_per_second
    double seconds_per_tick;

    //! @brief This is the inverse value of (::software_timer_timer_info_s::capture_compare + 1)
    double capture_compare_inverse;

} software_timer_timer_info_t;


//! @brief Timestamp for a specific point in time
typedef struct software_timer_timestamp_s
{
    //! @brief The automatically incremented hardware counter value
    uint16_t counter;

    //! @brief The interrupt increased counter overflow value
    uint64_t overflows;

    //! @brief Pointer to the timer to which the values refer
    software_timer_timer_info_t * timer_info;

} software_timer_timestamp_t;


//! @brief The object data of the software timer
typedef struct software_timer_s
{
    //! @brief Counter which must be reached in combination with
    //! software_timer_s::end_overflows for the timer to expire
    uint16_t end_counter;

    //! @brief The overflow of the counter which must be reached in combination
    //! with ::software_timer_t::end_counter for the timer to expire
    uint64_t end_overflows;

    //! @brief The duration of the counter after which the timer expires
    uint16_t duration_counter;

    //! @brief The duration of the overflows, after the timer expires
    uint64_t duration_overflows;

    //! @brief Duration of the timer in seconds
    double time_in_seconds;

    //! @brief Inverse value of ::software_timer_s::time_in_seconds
    double ticks_per_second;

    //! @brief Pointer to the timer to which the values refer
    software_timer_timer_info_t * timer_info;

    //! @brief Function that is called after the timer has expired
    software_timer_handler_t tick;

}software_timer_t;


/*---------------------------------------------------------------------*
 *  public: extern variables
 *---------------------------------------------------------------------*/
/*---------------------------------------------------------------------*
 *  public: function prototypes
 *---------------------------------------------------------------------*/

//! @brief Sets the duration of the specified timer object according to the specified time
//!
//! @param[in,out] object The software timer object
//! @param time_in_seconds The time after which the timer expires
//! @return Returns the flags with information about the calculated duration
software_timer_duration_flag_t software_timer_calculate_and_set_duration (software_timer_t * object, double time_in_seconds);

//! @brief  Checks if the timer is elapsed
//!
//! @details The handler assigned to the function pointer ::software_timer_t::tick is called.
//! If the function is used ensure that the hardware timer interrupt can be executed,
//! otherwise the ::software_timer_timer_info_s::overflow counter will not be incremented.
//!
//! If the function has not been called up for a long time and the timer would have been
//! triggered several times. The function will increment the end value only according to
//! the duration so that all triggers are called. This can lead to the timer being called
//! up very quickly one after the other until the end value is in the future again.
//!
//! If this behavior is not desired, the following function can be called:
//! ::software_timer_elapsed_prevent_multiple_triggers()
//!
//! @param[in,out] object The software timer object
//! @retval true  when the timer has elapsed
//! @retval false if the timer has not yet expired
bool software_timer_elapsed (software_timer_t *object);

//! @brief Checks if the timer is elapsed and prevent multiple triggers
//!
//! @details If the function has not been called up for a long time and the timer would
//! have been triggered several times. Then the function will ignore all triggers in
//! between and set the next trigger value in the future. The interval is retained.
//!
//! If this behavior is not desired, the following function can be called:
//! ::software_timer_elapsed()
//!
//! @param[in,out] object The software timer object
//! @retval true  when the timer has elapsed
//! @retval false if the timer has not yet expired
bool software_timer_elapsed_prevent_multiple_triggers (software_timer_t *object);

//! @brief Converts the timestamp value into a seconds value
//!
//! @param[in,out] timestamp Pointer to the timer values
//! @return Returns the time in seconds
double software_timer_get_time (software_timer_timestamp_t * timestamp);

//! @brief Converts the timestamp to the standard structure timespec
//!
//! @param[out] result_timespec Standard ::timespec struct.
//! @param[in] timestamp of the software timer.
void software_timer_get_timespec (struct timespec * result_timespec, software_timer_timestamp_t * timestamp);

//! @brief Reads the current values of the underlying timer
//!
//! @param[in,out] object The software timer object
//! @param[out] timestamp Pointer to the structure in which the values are to be saved
void software_timer_get_timestamp (software_timer_t * object, software_timer_timestamp_t * timestamp);

//! @brief Initializes the timer, normally it is better to use ::SOFTWARE_TIMER_INIT_HALT()
//!
//! @param[out] object The software timer object
//! @param[in] timer_info Address of the underlying hardware timer
void software_timer_init_halt (software_timer_t * object, software_timer_timer_info_t * timer_info);

//! @brief Checks if the timer is running
//!
//! @param[in] object The software timer object
//! @retval true  when the timer is running
//! @retval false if the timer is stopped
bool software_timer_is_running (software_timer_t * object);

//! @brief Checks if the timer is stopped
//!
//! @param[in] object The software timer object
//! @retval true  when the timer is stopped
//! @retval false if the timer is running
bool software_timer_is_stopped (software_timer_t * object);

//! @brief Starts the timer
//!
//! @param[in,out] object The software timer object
void software_timer_start (software_timer_t *object);

//! @brief Stops the software timer
//!
//! @param[in,out] object The software timer object
void software_timer_stop (software_timer_t *object);

//! @brief Calculates `result_and_minuend = result_and_minuend - subtrahend`
//!
//! @param[in,out] result_and_minuend Minuend
//! @param[in] subtrahend Subtrahend
void software_timer_sub_timestamp (software_timer_timestamp_t * result_and_minuend, software_timer_timestamp_t * subtrahend);


/*---------------------------------------------------------------------*
 *  public: static inline functions
 *---------------------------------------------------------------------*/

//! @brief To speed up the ::software_timer_set_duration() function, the values can be calculated in advance
//!
//! @param[out] object The software timer object
//! @param time_in_seconds The time after the timer expires
//! @param ticks_per_second The reciprocal of ::software_timer_s::time_in_seconds
//! @param duration_counter The calculated duration of the counter
//! @param duration_overflows The calculated duration of the overflows
INLINE void SOFTWARE_TIMER_SET_DURATION (software_timer_t * object, double time_in_seconds, double ticks_per_second, uint16_t duration_counter, uint64_t duration_overflows)
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
