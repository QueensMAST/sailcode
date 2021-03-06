#ifndef ANMEA_H_
#define ANMEA_H_

#include <Arduino.h>

#include <bstrlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// TODO check against NMEA 0183 standard
#define ANMEA_POLL_MAX_STRING_LEN 80



//NMEA string validity.
typedef enum {
    ANMEA_STRING_VALID,
    ANMEA_STRING_INVALID
};

typedef struct bstrList* blist;

/** Device specific state
 *
 * For the AIRMAR PB100
 */
typedef struct {
    Stream* port;

    uint8_t is_broadcasting;
    uint16_t baud;

    uint8_t mode;

    char target[6];
} anmea_airmar_t;

//NMEA tag searching modes.
typedef enum {
    ANMEA_BUF_COMPLETE,
    ANMEA_BUF_MATCH,
    ANMEA_BUF_SEARCHING,
    ANMEA_BUF_BUFFERING
} anmea_buffer_state_t;

// A buffer for holding an NMEA string data as it comes in.
typedef struct {
    uint8_t state;
    bstring data;
} anmea_buffer_t;

/** WIWMV Tag type
 *
 *  Flags used to track boolean values assciated with some fields
 */
typedef enum {
    ANMEA_TAG_WIMV_SPEED_VALID = 0x1,
    ANMEA_TAG_WIMV_DATA_VALID = 0x2,
    ANMEA_TAG_WIMV_WIND_RELATIVE = 0x4
} anmea_tag_wiwmv_flags_t;

//WIWMV (Wind) Tag
typedef struct {
    uint16_t wind_angle;
    uint16_t wind_speed;
    uint8_t flags;			//Flag, Relative or Theoretical

    // Time updated
    uint32_t at_time;
} anmea_tag_wiwmv_t;

//GPGLL (GPS) Tag
typedef struct {
    uint32_t longitude;
    uint32_t latitude;

    // Time updated
    uint32_t at_time;
} anmea_tag_gpgll_t;

//HCHDG (Heading) Tag
typedef struct {
    uint16_t mag_angle_deg;	// Magnetic heading angle, degrees x 10
	
    uint32_t at_time;		// Time updated
} anmea_tag_hchdg_t;

// Utility functions

// Build a string in a given buffer
void anmea_poll_string( Stream*, anmea_buffer_t*, const char* );

//Reset the buffer
void anmea_poll_erase( anmea_buffer_t* );

/** Performs checksum on string
 *
 * Returns:
 *  ANMEA_STRING_VALID
 *  ANMEA_STRING_INVALID
 */
uint8_t anmea_is_string_invalid( bstring );

// Tag specific functions

/** Update the WIWMV tag with values from a _valid_ string
 *
 * The string MUST have already been validated.
 */
void anmea_update_wiwmv( anmea_tag_wiwmv_t*, bstring );
void anmea_print_wiwmv( anmea_tag_wiwmv_t*, Stream* );

void anmea_update_hchdg( anmea_tag_hchdg_t*, bstring );
void anmea_print_hchdg( anmea_tag_hchdg_t*, Stream* );

void anmea_update_gpgll( anmea_tag_gpgll_t*, bstring );
void anmea_print_gpgll( anmea_tag_gpgll_t*, Stream* );

#endif
