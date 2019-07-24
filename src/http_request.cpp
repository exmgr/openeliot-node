#include "http_request.h"

// TODO: Comment everything

HttpRequest::HttpRequest(TinyGsm *modem, const char *server)
{
	_modem = modem;
	_server = (char*)server;
}

RetResult HttpRequest::get(const char *path, char *resp_buff, int resp_buff_size)
{
	return req(METHOD_GET, path, resp_buff, resp_buff_size, NULL, NULL, NULL);
}

RetResult HttpRequest::post(const char *path, const unsigned char *body, int body_len, char *content_type, 
	char *resp_buff, int resp_buff_size)
{
	return req(METHOD_POST, path, resp_buff, resp_buff_size, body, body_len, content_type);
}

RetResult HttpRequest::req(Method method, const char *path, char *resp_buff, int resp_buff_size,
	const unsigned char *body, int body_len, char *content_type)
{
    TinyGsmClient client(*_modem);
    HttpClient http_client(client, _server, _port);

	Serial.print(F("Request to: "));
	Serial.print(_server);
	Serial.println(path);
	Serial.print(F("Port: "));
	Serial.println(_port, DEC);

	if(!_modem->isGprsConnected())
    {
        Serial.println(F("GPRS is not connected, request aborted."));
        return RET_ERROR;
    }

	int ret = 0;
	int tries = GSM_TRIES;
	do
	{
		if(method == METHOD_GET)
		{
			ret = http_client.get(path);
		}
		else if(method == METHOD_POST)
		{
			ret = http_client.post(path, content_type, body_len, body);
		}

		if(ret == 0)
		{
			// All good
			break;
		}
		else
		{
			Serial.print(F("Request failed. "));
			if(tries > 1)
			{
				Serial.print(F("Toggling GPRS and retrying..."));

				// TODO: Not a good idea to control anthing through this class
				GSM::enable_gprs(false);
				delay(500);
				GSM::enable_gprs(true);
			}
			else
			{
				Serial.println(F("Aborting."));	
			}
			Serial.println();
		}
	}while(--tries);
    
    if(ret != 0)
    {
        Serial.print(F("Could not execute request. Error: "));
        Serial.println(ret, DEC);
        return RET_ERROR;
    }

    _response_code = http_client.responseStatusCode();
    Serial.print(F("Response code: "));
    Serial.println(_response_code, DEC);
    if(!_response_code)
    {
        Serial.println(F("Could not get response code."));
        return RET_ERROR;
    }

    int content_length = http_client.contentLength();

    if(content_length == 0)
    {
        Serial.println(F("Empty response."));
        return RET_OK;
    }
    else
    {
        Serial.print(F("Content length: "));
        Serial.println(content_length, DEC);
    }

	int bytes_read = 0;
	if(resp_buff != NULL && resp_buff_size > 1)
	{   
		// Read until buffer filled or all bytes read (which is smaller)
		int bytes_to_read = content_length < resp_buff_size ? content_length : resp_buff_size;

		bytes_read = http_client.readBytes(resp_buff, bytes_to_read);

		if(bytes_read != bytes_to_read)
		{
			Serial.print(F("Partial response read. Should be: "));
			Serial.print(bytes_to_read, DEC);
			Serial.print(F(" - Read: "));
			Serial.println(bytes_read);
		}


		if(bytes_read == resp_buff_size)
		{
			resp_buff[resp_buff_size - 1] = '\0';
		}
		else
		{
			resp_buff[bytes_read] = '\0';
		}
	}

    http_client.stop();

    _response_length = bytes_read;
    
    return RET_OK;
}

uint16_t HttpRequest::get_response_code()
{
	return _response_code;
}

int HttpRequest::get_response_length()
{
	return _response_length;
}

RetResult HttpRequest::set_port(int port)
{
	_port = port;
}