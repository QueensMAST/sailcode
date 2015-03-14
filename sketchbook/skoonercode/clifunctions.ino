/** Locally defined command line accessible functions
 ******************************************************************************
 */
/** Utility function
 * Checks to see if the bstring matches a given constant string without making
 * a copy of the bstrings contents.
 *
 * There is no guarantee that the char* in the bstring is valid or that the
 * given constant string will not contain anything weird
 *
 * Returns:
 * 1 If strings match
 * 0 Otherwise
 */
uint8_t
arg_matches( bstring arg, const char* ref )
{
    uint8_t match;

    match = strncasecmp(
            (char*) arg->data,
            ref,
            min( arg->slen, strlen(ref) )
        );

    // Return one if the string matches
    return match == 0 ? 1 : 0;
}

int cabout( blist list )
{
    Serial.println("GAELFORCE Skoonercode edition");
    Serial.println("Compiled " __DATE__ " " __TIME__ ".");
#ifdef DEBUG
    Serial.println(F("DEBUGGING BUILD"));
#endif

#ifndef _MEGA
    Serial.println(F("UNO BUILD: FUNCTIONS LIMITED"));
#endif

    char* buf;

    for( uint8_t i = 0; i < functions.index; i++ ) {
        Serial.print( (uint16_t) functions.cmds[i].func, HEX );
        Serial.print(F("-> "));

        buf = bstr2cstr( functions.cmds[i].name, '\0' );
        Serial.println( buf );
        free(buf);
    }
}

int cdiagnostic_report( blist list )
{
    diagnostics( &cli );

    return 0;
}

/** Test to check the encoder based scheduling
 *
 *  Starts executing the event queue until its finished. Loops forever until
 *  user presses q
 *
 *  -   Prints out some diagnostic info from the psched library every half
 *      second
 *  -   Constantly checking if target has been reached
 *  -   When target is reached, execute the event (stop the motor)
 *  -   Press q at any time to exit the infinite loop
 */
int ctest( blist list )
{
    static uint32_t last = 0;
    uint8_t check = 0;
    psched_dbg_dump_queue( &(penc_winch[1]), cli.port );

    while( cli.port->read() != 'q' ) {
        if( check == 1 ) {
            psched_exec_event( &(penc_winch[1]) );
            check = psched_advance_target( &(penc_winch[1]) );
            continue;
        }

        check = psched_check_target( &(penc_winch[1]) );
        if( last > millis() ) {
            continue;
        }

        last = millis() + 500;
        psched_dbg_print_pulses( &(penc_winch[1]), cli.port );
    }
}

/*** Monitor relay a serial connection back to serial 0
 *
 *   Assuming Serial is the connection to the user terminal, it takes all
 *   characters received on the specified serial connection and prints them.
 *
 *   Not tested for communication from user to device
 *
 *   Pressing q terminates monitor
 */
#ifdef _MEGA
int cmon( blist list )
{
    char* buf; // For parsing input number
    uint8_t target_port_number;
    Stream* target_port;

    char input = '\0'; // For mirroring data between ports

    if( !list || list->qty <= 1 ) return CONSHELL_FUNCTION_BAD_ARGS;

    buf = bstr2cstr( list->entry[1], '\0' );
    target_port_number = strtoul( buf, NULL, 10 );
    free(buf);

#ifdef DEBUG
    Serial.print("Using port ");
    Serial.println( target_port_number );
#endif

    if( target_port_number == 1 ) target_port = &Serial1;
    else if( target_port_number == 2 ) target_port = &Serial2;
    else if( target_port_number == 3 ) target_port = &Serial3;

    while( input != 'q' ) {
        if( target_port->available() >= 1 ) {
            Serial.print( (char) target_port->read() );
        }

        if( Serial.available() >= 1 ) {
            target_port->write( (input = Serial.read()) );
        }
    }
}
#endif

/*** Get the values of each channel as neutral value
 *
 * Sets the value of the global rc structure such that each new value read from
 * the current position of the sticks is saved as the neutral position for that
 * stick
 */
#ifdef RC_CALIBRATION
int calrc( blist list )
{
    cli.port->println(F(
                "Please setup controller by: \n"
                "   - Set all sticks to middle position\n"
                "   - Push enable switch away from you (down)\n"
                "   - Rudder knob to minimum\n"
                "Press enter to continue."
                ));
    while( cli.port->available() == 0 )
        ;
    if( cli.port->read() == 'q' ) return -1;

    // Left stick
    radio_controller.rsx.neutral =
        rc_get_raw_analog( radio_controller.rsx );
    radio_controller.rsy.neutral =
        rc_get_raw_analog( radio_controller.rsy );

    // Right stick
    radio_controller.lsx.neutral =
        rc_get_raw_analog( radio_controller.lsx );
    radio_controller.lsy.neutral =
        rc_get_raw_analog( radio_controller.lsy );

    // Switch and knob
    radio_controller.gear_switch.neutral =
        rc_get_raw_analog( radio_controller.gear_switch );
    radio_controller.aux.neutral =
        rc_get_raw_analog( radio_controller.aux );

    // Write the values to eeprom
    /*rc_write_calibration_eeprom( 0x08, &radio_controller );*/

    cli.port->println(F("Calibration values set"));
}
#endif // RC_CALIBRATION


/** Given a +/- and a mode name, apply or remove that mode bit
 *
 *  Valid mode names are
 *  -   RC      <- RC Control mode
 *  -   AIR     <- Polling AIRMAR for data
 *
 *  There must be a space between the +/- and modename
 */
int csetmode( blist list )
{
    unsigned int mode_bit = 0;
    char buf[50];

    if( list->qty <= 2 ) {
        return -1;
    }

    // Get the mode bit
    if(         arg_matches(list->entry[2], "air") ) {
        mode_bit = MODE_AIRMAR_POLL;
    } else if(  arg_matches(list->entry[2], "rc") ) {
        mode_bit = MODE_RC_CONTROL;
    } else if(  arg_matches(list->entry[2], "dia") ) {
        mode_bit = MODE_DIAGNOSTICS_OUTPUT;
    } else {
        // Do nothing and return
        return -2;
    }

    // Clear or set the bit
    if( arg_matches(list->entry[1], "set") ) {
        gaelforce |= mode_bit;
    } else {
        gaelforce &= ~(mode_bit);
    }

    snprintf_P( buf, sizeof(buf),
            PSTR( "MODE BIT %s: %d\n" ),
            arg_matches(list->entry[1], "set") ? "SET" : "UNSET",
            mode_bit );
    cli.port->print( buf );
    return 0;
}

/** Read write predetermined structures to and from eeprom
 *
 *  Uses two arguments, first is
 *      - r : for read
 *      - w : for write
 *
 *  Second is the item to be read/written, these structures are hardcoded in
 *  the sense that their eeprom address is written here. Available modules
 *
 *      - rc : RC calibration settings
 *      - ...
 *
 *      Returns -1 in case of error 0 otherwise
 */
int ceeprom( blist list )
{
    uint8_t rw = 0; // 0 is read-only
    bstring arg;

    if( list->qty <= 2 ) return -1;

    // Check the first argument
    arg = list->entry[1];
    if( arg_matches(arg, "w") ) {
        rw = 1;
    } else if( arg_matches(arg, "r") ) {
        rw = 0;
    } else {
#ifdef DEBUG
        Serial.print(F("USE EITHER r OR w"));
#endif
        return -1;
    }

    // Get the module name from the second argument
    arg = list->entry[2];
    if( arg_matches(arg, "rc") ) {
#ifdef DEBUG
        Serial.println(F("RC SETTINGS"));
#endif
        if( rw == 0 ) {
            rc_read_calibration_eeprom( 0x08, &radio_controller );
        } else if( rw == 1 ) {
            rc_write_calibration_eeprom( 0x08, &radio_controller );
        }

        rc_DEBUG_print_controller( &Serial, &radio_controller );

        return 0;
    } else {
#ifdef DEBUG
        Serial.print(F("NOT A MODULE"));
#endif
    }

    return -1;
}

int crcd( blist list )
{
    int16_t rc_in;

    while(      Serial.available() <= 0
            &&  Serial.read() != 'q' )
    {
        rc_in = rc_get_raw_analog( radio_controller.lsy );
        Serial.print(F( "L-Y: " ));
        Serial.print( rc_in );

        rc_in = rc_get_raw_analog( radio_controller.rsy );
        Serial.print(F( " R-Y: " ));
        Serial.print( rc_in );
		
		rc_in = rc_get_raw_analog( radio_controller.aux );
        Serial.print(F( " Aux: " ));
        Serial.println( rc_in );
    }
}

int ctest_pololu( blist list )
{
    uint16_t value = 100;

    pchamp_request_safe_start( &(pdc_winch_motors[0]) );
    pchamp_request_safe_start( &(pdc_winch_motors[1]) );

    pchamp_set_target_speed(
            &(pdc_winch_motors[0]), 1000, PCHAMP_DC_MOTOR_FORWARD );
    pchamp_set_target_speed(
            &(pdc_winch_motors[1]), 1000, PCHAMP_DC_MOTOR_FORWARD );

    value = pchamp_request_value(
                &(pdc_winch_motors[0]),
                PCHAMP_DC_VAR_VOLTAGE );
    Serial.print(F("Voltage 0: "));
    Serial.println( value );

    value = pchamp_request_value(
                &(pdc_winch_motors[1]),
                PCHAMP_DC_VAR_VOLTAGE );
    Serial.print(F("Voltage 1: "));
    Serial.println( value );

}

/** Primary motor control at command line
 *
 * Unlock the winch motors                          > mot u
 * Set the speed of a winch 0 to 2300 of 3200 units > mot g 0 2300
 * Set speed of winch in opposite direction         > mot g 0 -2300
 * Set rudder servo 0 to 1500us                     > mot r 0 1500
 * Halt winch motors and lock them                  > mot s
 *
 *mot w [-1000 to 1000] for winch
 *mot k [-1000 to 1000] for rudder (-600 to 600 is effective range)
 *
 * Current event based commands disabled
 */
int cmot( blist list )
{
    bstring arg;

    // Check if no actual arguments are given
    if( list->qty <= 1 ) {
        cli.port->print(F(
            "s - stop and lock all motors\n"
            "u - request unlock of both motors\n"
            "g (MOTOR) (SPEED) - set motor speed\n"
            "r (SERVO) (POS) - set rudder position\n"
            "e (MOTOR) (SPEED) (PULSE) - sched pulses from now\n"
            ));
        return -1;
    }

    // Check arguments
    arg = list->entry[1];
    if( arg_matches( arg, "s" ) ) {
        // Motors
        pchamp_set_target_speed(
                &(pdc_winch_motors[0]), 0, PCHAMP_DC_MOTOR_FORWARD );
        pchamp_request_safe_start( &(pdc_winch_motors[0]), false );
        pchamp_set_target_speed(
                &(pdc_winch_motors[1]), 0, PCHAMP_DC_MOTOR_FORWARD );
        pchamp_request_safe_start( &(pdc_winch_motors[1]), false );

        // Servos (disengage)
        pchamp_servo_set_position( &(p_rudder[0]), 0 );
        pchamp_servo_set_position( &(p_rudder[1]), 0 );
        cli.port->println(F("MOTORS LOCKED"));
    } else if( arg_matches( arg, "g" ) ) {
        if( list->qty <= 2 ) {
            cli.port->println(F("Not enough args"));
        }

        int8_t mot_num =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_num = constrain( mot_num, 0, 1 );

        int16_t mot_speed =
            strtol( (char*) list->entry[3]->data, NULL, 10 );
        mot_speed = constrain( mot_speed, -3200, 3200 );

        char buf[30];
        snprintf_P( buf, sizeof(buf),
                PSTR("Motor %d at %d"),
                mot_num,
                mot_speed
            );
        cli.port->println( buf );

        pchamp_set_target_speed(
                &(pdc_winch_motors[mot_num]),
                abs(mot_speed),
                mot_speed > 0 ? PCHAMP_DC_FORWARD : PCHAMP_DC_REVERSE
            );
    } else if( arg_matches( arg, "u" ) ) {
        pchamp_request_safe_start( &(pdc_winch_motors[0]) );
        pchamp_request_safe_start( &(pdc_winch_motors[1]) );
        cli.port->println(F("MOTORS UNLOCKED"));
    } else if( arg_matches( arg, "r" ) ) {
        if( list->qty <= 2 ) {
            cli.port->println(F("Not enough args"));
        }

        int8_t mot_num =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_num = constrain( mot_num, 0, 1 );

        int16_t mot_pos =
            strtol( (char*) list->entry[3]->data, NULL, 10 );
        mot_pos = constrain( mot_pos, 0, 6400 );

        char buf[30];
        snprintf_P( buf, sizeof(buf),
                PSTR("Servo %d to %d"),
                mot_num,
                mot_pos
            );
        cli.port->println( buf );

        pchamp_servo_set_position( &(p_rudder[mot_num]), mot_pos );
    } else if( arg_matches( arg, "e" ) ) {
        // Ewww, copy pasted code from a prev if case
        if( list->qty <= 3 ) {
            cli.port->println(F("Not enough args"));
        }

        int8_t mot_num =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_num = constrain( mot_num, 0, 1 );

        int16_t mot_speed =
            strtol( (char*) list->entry[3]->data, NULL, 10 );
        mot_speed = constrain( mot_speed, -3200, 3200 );

        uint16_t mot_enc =
            strtoul( (char*) list->entry[4]->data, NULL, 10 );

        // Nice basic case, start the motor with the target speed, then stop
        // after the specified number of pulses
        psched_new_target(
                &(penc_winch[1]),
                mot_speed,
                0 );

        psched_new_target(
                &(penc_winch[1]),
                0,
                mot_enc );
        cli.port->println(F("Events scheduled"));
    }
	
	else if( arg_matches( arg, "k" ) ){
		int16_t mot_pos =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_pos = constrain( mot_pos, -1000, 1000 );
		
		set_rudder(
                mot_pos,
                pdc_winch_motors,
                p_rudder
            );
	}
	
	else if( arg_matches( arg, "w" ) ){
		int16_t mot_pos =
            strtol( (char*) list->entry[2]->data, NULL, 10 );
        mot_pos = constrain( mot_pos, -1000, 1000 );
		
		set_winch(
                mot_pos,
                pdc_winch_motors,
                p_rudder
            );
	}
}

/** To get the time from the real time clock to set linux time
 *
 * Gets the number of seconds for the epoch, which can be used to set the time
 * on the clock of the raspberry pi.
 */
int cnow( blist list )
{
    cli.port->println(now());
}

int cres( blist list )
{
    reset_barnacle();
}

int cmovewinch(blist list) {
	if( list->qty <= 1 ) {
		cli.port->print(F("Please specify how far you want to move the winch."));
	return -1;
	} else {
		char buf[12];
		
		barn_clr_w1_ticks();
		int16_t offset =
			strtol( (char*) list->entry[1]->data, NULL, 10 );
		//cli.port->print(F("Offset is %d",offset));
		cli.port->print(F("Beginning to move"));
		if( offset == 0 ) cli.port->print(F("Offset is zero!"));
		if( offset > 0 ) {
			set_winch(500,pdc_winch_motors,p_rudder);
		} else if( offset < 0 ) {
			set_winch(-500,pdc_winch_motors,p_rudder);	
		}
		uint16_t tick_count = 0;
		while ( tick_count < abs(offset) ) {
			if(Serial.available() && Serial.read() == 'q') {
				cli.port->print(F("Pressed Q!"));
				break;
			}
			tick_count = barn_get_w1_ticks();
			delay(10);
			snprintf_P( buf, sizeof(buf),
				PSTR("TICKS:%u\n"),
				tick_count
			);
			cli.port->print(buf);
			
		}
		set_winch(0,pdc_winch_motors,p_rudder);
	}
}

int cairmar(blist list){
	bstring arg = list->entry[1];
	if( arg_matches( arg, "poll" ) ){
		while(      Serial.available() <= 0
            &&  Serial.read() != 'q' ){
				if(SERIAL_PORT_AIRMAR.available())
					cli.port->print((char)SERIAL_PORT_AIRMAR.read());
			}
	}
	
	else if( arg_matches( arg, "en" ) ){
		arg = list->entry[2];
		if(arg_matches( arg, "wind" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,MWVR,1,5");
		else if(arg_matches( arg, "all" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,ALL,1,5");
		else if(arg_matches( arg, "head" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,HDG,1,5");
		else if(arg_matches( arg, "gps" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,GLL,1,5");
		cli.port->print("Sent to AIRMAR!");
		cli.port->println();
	}
	
	else if( arg_matches( arg, "dis" ) ){
		arg = list->entry[2];
		if(arg_matches( arg, "wind" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,MWVR,0,5");
		else if(arg_matches( arg, "all" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,ALL,0,5");
		else if(arg_matches( arg, "head" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,HDG,0,5");
		else if(arg_matches( arg, "gps" ))
			SERIAL_PORT_AIRMAR.println("$PAMTC,EN,GLL,0,5");
		cli.port->print("Sent to AIRMAR!");
		cli.port->println();
	}
	
}
/******************************************************************************
 * END OF COMMAND LINE FUNCTIONS */
// vim:ft=c:
