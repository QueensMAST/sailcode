#define AUTOSAIL_TIMEOUT 10000
#define AUTOSAIL_ALGORITHM_TICKS 100
#define AUTOSAIL_MIN_DELAY 5
#define NAV_MIN_DELAY 2000
#define devCount 360
#define CHDEG 35 //Closest degree offset from wind direction that boat can sail
#define DIRCONST 15 //Offset of green area from dirDeg (plus or minus)
#define IRON_CONST 15
#define TACK_TIMEOUT 5000
uint32_t autosail_start_time = 0;
uint32_t time_since_lastNav;
uint32_t last_degree;

//int timer_station = 0;
//uint32_t last_change = 0;


uint32_t head_check = 1000;
uint32_t rud_update = 1000;
int32_t angle_to_wp = 0;
int degree = 0;
int8_t turn_flag = 2;
uint32_t gps_update_time = 0;
TinyGPSPlus gps;
TinyGPSCustom wind_s(gps, "WIMWV", 3);
TinyGPSCustom wind_a(gps, "WIMWV", 1);
TinyGPSCustom head(gps, "HCHDG",1);

//const int winDeg = 0;
  
 typedef struct DegScore{
	int degree;
	int score;
}DegScore;

int32_t tack_time = 0;
int target = 0;
uint32_t station_time = 1;
int rudder_takeover = 0;
uint32_t print_time = 0;

DegScore navScore[devCount];


void autosail_main(int auto_mode){
	//get_airmar_data();

	if ((millis() - autosail_start_time) <100){
		return;
	}
	
	

// Some sort of if for autosail modes, searching, stationkeep, race.
	autosail_start_time = millis();
	//static uint32_t tick_counter = 0;
	
	

	
	/*const int UPangle = 45;
	int winch_update_time;
	int winch_wind = wind_tag.wind_angle;
	//Winch update, using 45 degree threshold for in
	if ((millis() - winch_update_time) < 5000){
		winch_update_time = millis();
		if(winch_wind > 180){
			winch_wind = 360 - winch_wind;
		}
		//All in if close hauled
		if(winch_wind < UPangle){
			motor_winch_abs(-1000);
		}
		//Map to point of sail
		else{
			motor_winch_abs(map(winch_wind,45,180,-1000,1000));
		}
	}
	*/
	if(autosail_check_timeout()){
		//tick_counter++;
		return;
	}
	int autosail_mode = 2;//auto_mode;
        
	//Stationkeeping
	if(autosail_mode == 1){	
		XBEE_SERIAL_PORT.println("Station!");
		stationKeep();
	}
	
	//Computer vision tracking
	if(autosail_mode == 2){
		XBEE_SERIAL_PORT.println("Comp Vi");
		rudder_takeover = 0;
		int way_order[9] = {1,8,6,2,4,9,3,5,7};
		int tracker = 0;

		// Get ball position from Odroid's GPIO pins
		int leftPin = digitalRead(ODROID_COMPVI_LEFT);
		int rightPin = digitalRead(ODROID_COMPVI_RIGHT);
		
		rudder_takeover = leftPin || rightPin;
		
		if(leftPin && rightPin) {
			XBEE_SERIAL_PORT.println("Ball straight ahead");
		} else if(leftPin) {
			changeDir(270);
			XBEE_SERIAL_PORT.println("Ball to the left");
		} else if(rightPin) {
			changeDir(90);
			XBEE_SERIAL_PORT.println("Ball to the right");
		} else {
			XBEE_SERIAL_PORT.println("Ball not found");
		}
		
		if(target_wp != way_order[tracker]){
			tracker++;
			tracker = tracker % 9;
			target_wp = way_order[tracker];
		}
	}
	/*if(rudder_takeover == 0){
		if(((millis() - head_check) > 500)){
			angle_to_wp = (720 + (gpsDirect() -(head_tag.mag_angle_deg/10)) + 12)%360;

			head_check = millis();
		}
		if(((millis() - rud_update) > 500)){
			degree = calcNavScore(wind_tag.wind_angle,angle_to_wp);
			changeDir(degree);
			rud_update = millis();
		}
	}*/

						
}
/**
* Changes the rudders based on a given degree from bow
**/
void changeDir(int degree){

	XBEE_SERIAL_PORT.print("Degree: ");
	XBEE_SERIAL_PORT.println(degree);
//	XBEE_SERIAL_PORT.print("Target: ");
//	XBEE_SERIAL_PORT.print(target);
		
	if (degree > 5 && degree <= 180){
		XBEE_SERIAL_PORT.println(F("Turning RIGHT\n"));

		if(target > 0){
			target = target - 150;
			tack_time = millis();
		}
		else if(target > -1000){
			target = target - 75;
			tack_time = millis();
		}
		else{
			if(millis() - tack_time > TACK_TIMEOUT){
				target = 1000;
				tack_time = millis();
					/*tack_try_count++;
					if (tack_try_count == 3){
						tack_try_count = 0;
						delay(5000); //jybe
					}*/
			}
		}
	}
	else if(degree > 180 && degree < 355){
		XBEE_SERIAL_PORT.println(F("Turning LEFT\n"));

		if(target < 0){
			target = target + 150;
			tack_time = millis();
		}
		else if(target < 1000){
			target = target + 75;
			tack_time = millis();
		}
		else{
			if(millis() - tack_time > TACK_TIMEOUT){
					target = -1000;
					tack_time = millis();
					/*tack_try_count++;
					if (tack_try_count == 3){
						tack_try_count = 0;
						delay(5000); //jybe
					}*/
			}
		}
	}
	else{
		XBEE_SERIAL_PORT.println(F("Sailing straight\n"));
		target = 0;
	}
	
	XBEE_SERIAL_PORT.print("Degree: ");
	XBEE_SERIAL_PORT.println(degree);
	motor_set_rudder(target);
}
void update_airmar_tags(){
	gps_tag.latitude = gps.location.lat() * 1000000;
	gps_tag.longitude = gps.location.lng() * -1000000;
	wind_tag.wind_angle = (uint16_t)
       ( strtod( (const char*) 	wind_a.value(), NULL ));
	wind_tag.wind_speed =  (uint16_t)
       ( strtod( (const char*) 	wind_s.value(), NULL ));
	head_tag.mag_angle_deg =  (uint16_t)
       ( strtod( (const char*) 	head.value(), NULL )) *10;
	if(millis() - print_time > 500)
	{
		print_time = millis();
		/*Serial.print(F("Lat="));   Serial.print(gps_tag.latitude); 
		Serial.print(F(" Long=")); Serial.print(gps_tag.longitude); 
		Serial.print(F(" Wind Angle=")); Serial.print(wind_tag.wind_angle); 
		Serial.print(F(" Wind Speed=")); Serial.print(wind_tag.wind_speed);
		Serial.print(F(" Heading=")); Serial.println(((head_tag.mag_angle_deg/10)+12)%360);*/
		XBEE_SERIAL_PORT.print("LAT: "); XBEE_SERIAL_PORT.println(gps_tag.latitude);
		XBEE_SERIAL_PORT.print("LONG: "); XBEE_SERIAL_PORT.println(gps_tag.longitude);
		XBEE_SERIAL_PORT.print("Heading: "); XBEE_SERIAL_PORT.println(((head_tag.mag_angle_deg/10)+12)%360);
		XBEE_SERIAL_PORT.print("Degree from bow to wp: "); XBEE_SERIAL_PORT.println(angle_to_wp);
    }
}
/*void update_airmar_tags(){

	anmea_poll_string(
			&SERIAL_PORT_AIRMAR,
			&airmar_buffer,
			AIRMAR_TAGS[airmar_turn_counter]
		);
   	//XBEE_SERIAL_PORT.println(airmar_buffer.state);
	//Serial.print(airmar_buffer.state);
	if( airmar_buffer.state == ANMEA_BUF_MATCH ) {
		XBEE_SERIAL_PORT.println(F("MATCH\n"));
		Serial.println(F("MATCH\n"));
		if(airmar_turn_counter==0){
			anmea_update_hchdg(&head_tag, airmar_buffer.data);
			XBEE_SERIAL_PORT.println("HEADUPDATE!");
		}
		
		else if(airmar_turn_counter==1){
			anmea_update_wiwmv(&wind_tag, airmar_buffer.data);
			XBEE_SERIAL_PORT.println("WINDUPDATE!");
		}
		
		if((millis() - gps_update_time) > 1000000){
			anmea_update_gpgll(&gps_tag, airmar_buffer.data);
			gps_update_time = millis();
			XBEE_SERIAL_PORT.println("GPSUPDATE!");
			Serial.println("GPSUPDATE!");
		}
		
		airmar_turn_counter++;
		airmar_turn_counter = airmar_turn_counter%2;
		
		//autosail_print_tags();
		
		anmea_poll_erase( &airmar_buffer );
	}
}*/


uint8_t autosail_check_timeout(){
	if((millis() - autosail_start_time) > AUTOSAIL_TIMEOUT){
		XBEE_SERIAL_PORT.print(F("Autosail Timeout\n"));
		return 1;
	}
	else
		return 0;
}


void autosail_print_tags(){
	anmea_print_wiwmv(&wind_tag, &XBEE_SERIAL_PORT);
	anmea_print_hchdg(&head_tag, &XBEE_SERIAL_PORT);
	anmea_print_gpgll(&gps_tag, &XBEE_SERIAL_PORT);
	XBEE_SERIAL_PORT.print("\n");
}
void
auto_update_winch(
        rc_mast_controller* rc,
        pchamp_controller mast[2])
{
    int16_t rc_input;

    char buf[40];       // buffer for printing debug messages
    uint16_t rvar = 0;  // hold result of remote device status (pololu controller)

    // Track positions from last call, prevent repeat calls to the same
    // position.

    static uint16_t old_winch = 0;
    
	//Winch
	rc_input = rc_get_mapped_analog( rc->lsy, -1200, 1200 );
	if(abs(rc_input)<200)
		rc_input = 0;
	rc_input = map(rc_input, -1200, 1200, -1000, 1000);
	
	if(abs(old_winch - rc_input)>=25){
		motor_set_winch(
                rc_input
            );
		old_winch = rc_input;
	}

}

//Calculates best navscore, and returns it's degree
int calcNavScore(int windDeg, int dirDeg){ 
 //wind deg is degree of wind (clockwise) with respect to boat's bow (deg = 0). 
 // dirDeg is degree of waypoint (clockwise) with respect to boat's bow (deg = 0).
	int best_score = 1000;
	int best_degree;
	for (int i = 0; i < devCount; i++){
		navScore[i].degree = 360 * i / devCount;
		navScore[i].score = calcWindScore(windDeg, navScore[i].degree) + calcDirScore(dirDeg, navScore[i].degree);
		if (navScore[i].score < best_score){
			best_score = navScore[i].score;
			best_degree = navScore[i].degree;
		}
	}
	if (abs(windDeg - dirDeg) > CHDEG && abs(360 - windDeg + dirDeg) > CHDEG && abs(360 + windDeg - dirDeg) > CHDEG){
	//check if waypoint is down wind from close hauled limit
			best_degree = dirDeg;
	}
	return (best_degree);
}

int calcWindScore(int windDeg, int scoreDeg){
	//calculate red area
	if (abs(windDeg - scoreDeg) < CHDEG) //check if facing upwind
		return 360; //return high number for undesirable direction (upwind)
	
	int pointA = (windDeg + 180) % 360; //Point directly down wind from boatA
	int pointB = (windDeg + CHDEG) % 360; //Maximum face-up angle (clockwise from wind)
	int pointC = (windDeg - CHDEG) % 360; //Maximum face-up angle (counter-clockwise from wind)

	if (abs(windDeg - scoreDeg)  <= CHDEG ||
	abs(360 - windDeg + scoreDeg) <= CHDEG || 
	abs(360 + windDeg - scoreDeg) <= CHDEG) //check if facing upwind
				return 170;	
	//Calculate green area if wind is to port (left) of bow
	else if(windDeg == 0)
		return 10;
	else if (windDeg > 180 && (scoreDeg <= pointA || scoreDeg >= pointB))
		return 10; //arbitrary
	//Calculate yellow area if wind is to port (left) of bow
	else if (windDeg > 180 && (scoreDeg >= pointA && scoreDeg <= pointC))
		return 100;//arbitrary
	//Calculate green area if wind is to starboard (right) of bow
	else if (windDeg <= 180 && (scoreDeg >= pointA || scoreDeg <= pointC))
		return 10;//arbitrary
	//Calculate yellow area if wind is to starboard (right) of bow
	else if (windDeg <= 180 && (scoreDeg <= pointA && scoreDeg >= pointB))
		return 100;//arbitrary
}

int calcDirScore(int dirDeg, int scoreDeg){
	//green area
	if (abs(dirDeg - scoreDeg) < DIRCONST)
		return 2;//arbitrary
	//yellow area
	if (abs(dirDeg - scoreDeg) <= 90)
		return 5;//arbitrary
	//red area
	return 10;
	
}

void stationKeep(){
	if(station_time == 1){
		station_time = millis();
	}
	if(millis() - station_time > 300000){
		rudder_takeover = 1;
		changeDir(0);
	}
	else{
		num_of_wps = 10;
		//go between two pre determined points
		if(target_wp < 9){
			target_wp = 9;
		}
	}
}

int gpsDirect(){
	int lat_vect = waypoint[target_wp].lat - gps_tag.latitude;
	int lon_vect = waypoint[target_wp].lon + gps_tag.longitude;
	double gps_vect = (double)(lat_vect)/(double)(lon_vect);
	
	if(abs(lat_vect) < 50 && abs(lon_vect) <50 && lat_vect != 0 && lon_vect != 0){
		target_wp++;
		target_wp = target_wp%(num_of_wps);
		XBEE_SERIAL_PORT.println("Waypoint Reached! ");
		XBEE_SERIAL_PORT.print("New target: ");
		XBEE_SERIAL_PORT.println(target_wp);
		lat_vect = waypoint[target_wp].lat - gps_tag.latitude;
		lon_vect = waypoint[target_wp].lon + gps_tag.longitude;
		gps_vect = (double)(lat_vect)/(double)(lon_vect);
	}
	XBEE_SERIAL_PORT.print("Angle to waypoint: ");
	XBEE_SERIAL_PORT.println(atan (gps_vect)*(180/PI));
	XBEE_SERIAL_PORT.print("North: ");
	XBEE_SERIAL_PORT.println(lat_vect);
	XBEE_SERIAL_PORT.print("East: ");
	XBEE_SERIAL_PORT.println(lon_vect);

	if(lat_vect < 0 && lon_vect < 0){	
		return(270 - (atan (lat_vect/lon_vect)*(180/PI))); 
	}
	if(lat_vect < 0 && lon_vect > 0){	
		return(90 + abs(atan (lat_vect/lon_vect)*(180/PI))); 
	}
	if(lat_vect > 0 && lon_vect < 0){	
		return(270 + abs (atan (lat_vect/lon_vect)*(180/PI))); 
	}
	if(lat_vect > 0 && lon_vect > 0){	
		return(90 -(atan (lat_vect/lon_vect)*(180/PI))); 
	}
}

void get_airmar_data(){
	gps_update_time = millis();
	while( (!gps.location.isUpdated() || wind_s.isUpdated() ||
			!wind_a.isUpdated() || head.isUpdated())
			&& ( millis() - gps_update_time < 6000)){
		if (Serial2.available()){
			gps.encode(Serial2.read());
		}
	}
	if(millis() - gps_update_time < 6000){
		update_airmar_tags();
	}
	else{
		XBEE_SERIAL_PORT.listen();
		XBEE_SERIAL_PORT.println(F("Cannot connect to Airmar"));
	}
}