/**
*	This class assumes that a linear potentiometer is used to encode the position
*	of the canted keel.
*/

#ifndef CANTED_KEEL_h
#define CANTED_KEEL_h

#include <pololu_champ.h>

class Canted_Keel
{
public:
	/**
	*	@brief Constructor for Canted_Keel object.  Must declare in void setup().
	*	@param pololu_num ID set on Pololu motor controller.
	*	@param pololu_serial Serial port that pololu is connected to.
	*	@param potentiometer_pin Pin that potentiometer is connected to.
	*	@param max_potentiometer_value Voltage of potentiometer when keel is as far to the starboard side as possible (volts).
	*	@param min_potentiometer_value Voltage of potentiometer when keel is as far to the port side as possible (volts).
	*	@param center_potentiometer_value Voltage of potentiometer when keel is centered under hull (volts).
	*	@param max_starboard_angle Maximum absolute angle keel can displace to the starboard side with respect to the center position (degrees).
	*	@param max_port_angle Maximum absolute angle keel can displace to the port side with respect to the center position (degrees).
	*/
	Canted_Keel(
				uint8_t pololu_num, 
				Stream* pololu_serial, 
				int potentiometer_pin, 
				double max_potentiometer_voltage, 
				double min_potentiometer_voltage,
				double center_potentiometer_voltage,
				double max_starboard_angle,
				double max_port_angle
				);
				
	/**
	*	@brief Destructor for Canted_Keel object
	*/
	~Canted_Keel();
	
	/**
	*	@brief Returns the keel's current position.  Starboard is positive angle, port is negative angle (degrees)
	*/
	double getPosition();
	
	/**
	*	@brief Sets the keel's position;
	*	@param new_position Angle at which to position keel (degrees)
	*	@param motor_speed Absolute speed of keel (0 to 1000)
	*/
	bool setPosition(double new_position, int motor_speed);
	
private:
	// Attributes
	pchamp_controller m_keel_control;
	int m_potentiometer_pin;
	double m_max_potentiometer_value; //10-bit A-to-D voltage (range 0 - 1024, representing 0 - % volts)
	double m_min_potentiometer_value; //10-bit A-to-D voltage
	double m_center_potentiometer_value; //10-bit A-to-D voltage
	double m_max_potentiometer_resistance; //Ohms
	double m_min_potentiometer_resistance; //Ohms
	double m_center_potentiometer_resistance; //Ohms
	double m_max_starboard_angle; //in degrees
	double m_max_port_angle; //in degrees
	
	//Functions
	/**
	*	@brief Lock keel's motor's position.
	*/
	void motor_lock();
	
	/**
	*	@brief Enable keel motor.
	*/
	void motor_unlock();
	
	/**
	*	@brief Calculates the R2 resistance of a voltage divider given all other parameters
	*	@param vin Input voltage to voltage divider (volts)
	*	@param vout Output voltage of voltage diviter (volts)
	*	@param r1 R1 resistance in voltage diviter (ohms)
	*/
	double findVoltageDividerR2 (int vin, int vout, int r1);
};

#endif