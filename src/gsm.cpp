#include <ctime>
#include <string>
#include "app_config.h"
#include "const.h"
#include <TinyGsmClient.h> // Must be after const.h where tiny gsm constants are set
#include "gsm.h"
#include "rtc.h"
#include "struct.h"
#include "utils.h"
#include "log.h"
#include <Wire.h>

#define LOGGING 1
#include <ArduinoHttpClient.h>

#define _gsm_serial Serial1

namespace GSM
{
//
// Private vars
//
/** Hardware serial port to be used by the GSM lib */
// HardwareSerial _gsm_serial(GSM_SERIAL_PORT);

/** TinyGSM instance */
#if PRINT_GSM_AT_COMMS
#include <StreamDebugger.h>
/** StreamDebugger object to ouput communication between GSM module and MCU to serial console */
StreamDebugger debugger(_gsm_serial, Serial);
TinyGsm _modem(debugger);
#else
TinyGsm _modem(_gsm_serial);
#endif

//
// Private functions
//
void power_cycle();
bool is_fona_serial_open();

/******************************************************************************
 * Init GSM functions 
 ******************************************************************************/
void init()
{
	pinMode(PIN_GSM_PWR_KEY, OUTPUT);
	pinMode(PIN_GSM_RESET, OUTPUT);
	pinMode(PIN_GSM_POWER_ON, OUTPUT);

	// Make sure modem is off
	off();
}

/*****************************************************************************
* Power ON and connect
*****************************************************************************/
RetResult on()
{
	Serial.print(F("GSM ON"));
	Serial.println();
	int time_delay = 500;

	#ifdef TCALL_H
		// Turn IP5306 power boost OFF to reduce idle current
		Utils::ip5306_set_power_boost_state(true);
	#endif
	
	digitalWrite(PIN_GSM_RESET, HIGH);
	delay(time_delay);

	// Make sure modem is OFF
	// Serial.println("PIN_GSM_PWR_KEY: HIGH");
	digitalWrite(PIN_GSM_PWR_KEY, HIGH);

	// Enable SY8089 4V4 for SIM800 
	// Serial.println("PIN_GSM_POWER_ON: HIGH");
	digitalWrite(PIN_GSM_POWER_ON, HIGH);
	delay(time_delay);
	delay(time_delay);

	digitalWrite(PIN_GSM_PWR_KEY, LOW); // Turn modem ON
	delay(time_delay);

	_gsm_serial.end(); // Could be related to meditation guru error
	delay(10);

	_gsm_serial.begin(GSM_SERIAL_BAUD, SERIAL_8N1, PIN_GSM_RX, PIN_GSM_TX);
	delay(3000);

	//_modem.restart();
	Serial.println("Modem init...");

	if (!_modem.init())
	{
		Log::log(Log::GSM_INIT_FAILED);
		Serial.println(F("Could not init."));
		return RET_ERROR;
	}

	String modem_info = _modem.getModemInfo();
	Serial.print(F("GSM module: "));
	Serial.println(modem_info.c_str());

	return RET_OK;
}

/*****************************************************************************
* Power OFF
*****************************************************************************/
RetResult off()
{
	digitalWrite(PIN_GSM_PWR_KEY, HIGH); // Turn off modem in case its ON from previous state
	digitalWrite(PIN_GSM_POWER_ON, LOW); // turn off modem power in case its from previous state
	digitalWrite(PIN_GSM_RESET, HIGH);   // Keep IRQ high anyway (or not to save power?)

	#ifdef TCALL_H
		// Turn IP5306 power boost OFF to reduce idle current
		Utils::ip5306_set_power_boost_state(false);
	#endif

	return RET_OK;
}

/******************************************************************************
 * Connect to the network and enable GPRS
 *****************************************************************************/
RetResult connect()
{
	Serial.println(F("Initializing GSM"));

	Serial.println(F("Waiting for network connection..."));
	if (!_modem.waitForNetwork(GSM_DISCOVERY_TIMEOUT_MS))
	{
		Serial.println(F("Network discovery failed."));
		Log::log(Log::GSM_NETWORK_DISCOVERY_FAILED);
		return RET_ERROR;
	}

	Serial.println(F("GSM connected"));

	int8_t rssi = get_rssi();
	Utils::serial_style(STYLE_BLUE);
	Serial.print(F("RSSI: "));
	Serial.println(rssi, DEC);
	Utils::serial_style(STYLE_RESET);

	if (enable_gprs(true) != RET_OK)
	{
		Log::log(Log::GSM_GPRS_CONNECTION_FAILED);
		return RET_ERROR;
	}

	return RET_OK;
}

/******************************************************************************
 * Calls connect() X times until it succeeds and power cycles GSM module
 * inbetween failures
 ******************************************************************************/
RetResult connect_persist()
{
	int tries = GSM_TRIES;
	int success = false;

	// Try to connect X times before aborting, power cycling in between
	while (tries--)
	{
		if (connect() == RET_ERROR)
		{
			Serial.print(F("Connection failed. "));

			if (tries)
			{
				Serial.println(F("Cycling power and retrying."));
			}
			power_cycle();
		}
		else
		{
			success = true;
			break;
		}
	}

	if (!success)
	{
		Serial.println(F("Could not connect. Aborting."));

		return RET_ERROR;
	}
	else
		return RET_OK;
}

/*****************************************************************************
* Power cycle
*****************************************************************************/
void power_cycle()
{
	off();
	delay(500);
	on();
}

/*****************************************************************************
* Update time from NTP
*****************************************************************************/
RetResult update_ntp_time()
{
	Serial.print(F("Updating time from: "));
	Serial.println(NTP_SERVER);

	// Toggle GPRS to make sure it returns NTP time and not GSM time
	// enable_gprs(false);
	// if (enable_gprs(true) != RET_OK)
	// 	return RET_ERROR;

	if (_modem.NTPServerSync(F(NTP_SERVER), 0) == -1)
	{
		return RET_ERROR;
	}

	return RET_OK;
}

/*****************************************************************************
* Get time from module
*****************************************************************************/
RetResult get_time(tm *out)
{
	char buff[26] = "";

	// Returned format: "30/08/87,05:49:54+00"
	String date_time = _modem.getGSMDateTime(DATE_FULL);

	Serial.print(F("Got time: "));
	Serial.println(date_time.c_str());

	if (date_time.length() < 1)
	{
		Serial.println(F("Could not get time from GSM."));
		return RET_ERROR;
	}

	if (0 == sscanf(date_time.c_str(), "%02d/%02d/%02d,%02d:%02d:%02d",
					&(out->tm_year), &(out->tm_mon), &(out->tm_mday),
					&(out->tm_hour), &(out->tm_min), &(out->tm_sec)))
	{
		Serial.print(F("Could not parse FONA time: "));
		Serial.println(buff);

		// 0 vars parsed
		return RET_ERROR;
	}

	// Year in GSM module counts from 2000, must count from 1900
	out->tm_year += 100;
	// Month in GSM module counts from 01, must count from 00
	out->tm_mon--;

	time_t gsm_epoch = mktime(out);
	Serial.print(F("Parsed time in GSM module: "));
	Serial.println(ctime(&gsm_epoch));

	return RET_OK;
}

/******************************************************************************
 * Enable/disable GPRS
 *****************************************************************************/
RetResult enable_gprs(bool enable)
{
	if (enable)
	{
		// Get APN from device descriptor
		const DeviceDescriptor *device_descriptor = Utils::get_device_descriptor();
		Serial.print(F("Connecting to APN: "));
		Serial.println(device_descriptor->apn);

		if (!_modem.gprsConnect(device_descriptor->apn, "", ""))
		{
			Serial.println(F("Could not connect GPRS."));
			return RET_ERROR;
		}
	}
	else
	{
		if (!_modem.gprsDisconnect())
		{
			Serial.println(F("Could not disable GPRS."));
			return RET_ERROR;
		}
	}

	Serial.println(F("GPRS connected."));

	return RET_OK;
}

/******************************************************************************
* Convert value returned by SIM into dBm and return
* @return RSSI in dBm
******************************************************************************/
int get_rssi()
{
	// TODO: Check if serial is open else get exception

	uint16_t rssi_sim = _modem.getSignalQuality();
	int16_t rssi = 0;

	if (rssi_sim == 0)
		rssi = -115;
	if (rssi_sim == 1)
		rssi = -111;
	if (rssi_sim == 31)
		rssi = -52;
	if ((rssi_sim >= 2) && (rssi_sim <= 30))
		rssi = map(rssi_sim, 2, 30, -110, -54);

	return rssi;
}

/******************************************************************************
 * Get battery info from module
 * @param 
 *****************************************************************************/
RetResult get_battery_info(uint16_t *voltage, uint16_t *pct)
{
	if (!(*voltage = _modem.getBattVoltage()))
	{
		*voltage = 0;
		Serial.println(F("Could not read voltage from GSM module"));
		return RET_ERROR;
	}
	

	if (!(*pct = _modem.getBattPercent()))
	{
		*pct = 0;
		Serial.println(F("Could not read battery pct from GSM module."));
		return RET_ERROR;
	}
	
	return RET_OK;
}

/******************************************************************************
 * Check if SIM card present by checking if its ready
 *****************************************************************************/
bool sim_card_present()
{
	return _modem.getSimStatus() == SIM_READY;
}

/******************************************************************************
 * Get TinyGsm object
 *****************************************************************************/
TinyGsm *get_modem()
{
	return &_modem;
}

} // namespace GSM