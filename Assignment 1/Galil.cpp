#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>
#include <bitset>
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#include <string.h>

#include "Galil.h"
#include "EmbeddedFunctions.h"

Galil::Galil() {
	Functions = new EmbeddedFunctions;
	g = 0;
	for (int i = 0; i < 1024; i++) {
		ReadBuffer[i] = { NULL };
	}
	for (int i = 0; i < 3; i++) {
		ControlParameters[i] = 0;
	}
	setPoint = 0;

}

Galil::Galil(EmbeddedFunctions* Funcs, GCStringIn address) {
	Functions = Funcs;
	g = 0;
	for (int i = 0; i < 1024; i++) {
		ReadBuffer[i] = { NULL };
	}
	for (int i = 0; i < 3; i++) {
		ControlParameters[i] = 0;
	}
	setPoint = 0;

	GReturn status = 0;
	status = Functions->GOpen(address, &g);
	/*if (status == G_NO_ERROR)
	{
		std::cout << "Connection Success!" << std::endl;
	}
	else {
		std::cout << "Connection Failed!" << std::endl;
	}*/
}


Galil::~Galil() {
	if (g) {
		Functions->GClose(g);
		delete Functions;
	}
	//std::cout << "Connection Closed!" << std::endl;
}


void Galil::DigitalOutput(uint16_t value) {
	char Command[128] = "";
	ReadBuffer[0] = { NULL };

	if (value < 256) {
		sprintf_s(Command, "OP %d,0;", value);
	}
	else if (value >= 256 && value < 65536) {
		int lowbyte = value % 256;
		int highbyte = value / 256;
		sprintf_s(Command, "OP %d,%d;", lowbyte, highbyte);
	}
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
}


void Galil::DigitalByteOutput(bool bank, uint8_t value) {
	char Command[128] = "";
	ReadBuffer[0] = { NULL };

	if (bank == 0) {
		sprintf_s(Command, "OP %d;", value);
	}
	else {
		sprintf_s(Command, "OP ,%d;", value);
	}
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
}

void Galil::DigitalBitOutput(bool val, uint8_t bit) {
	char Command[128] = "";
	ReadBuffer[0] = { NULL };

	if (val == 0) {
		sprintf_s(Command, "CB %d;", bit);
		Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
	}
	else {
		sprintf_s(Command, "SB %d;", bit);
		Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
	}
}


uint16_t Galil::DigitalInput() {
	double Input = 0;
	int j = 0;
	for (int i = 0; i < 16; i++) {
		Input += pow(2 * int(DigitalBitInput(i)), j);
		j++;
	}
	if (DigitalBitInput(0) == 0) {
		Input--;
	}
	return Input;
}


uint8_t Galil::DigitalByteInput(bool bank) {
	double Input = 0;
	int j = 0;
	if (bank == 0) {
		for (int i = 0; i < 8; i++) {
			Input += pow(2 * int(DigitalBitInput(i)), j);
			j++;
		}
		if (DigitalBitInput(0) == 0) {
			Input--;
		}
	}
	else {
		for (int i = 8; i < 16; i++) {
			Input += pow(2 * int(DigitalBitInput(i)), j);
			j++;
		}
		if (DigitalBitInput(8) == 0) {
			Input--;
		}
	}
	return Input;
}


bool Galil::DigitalBitInput(uint8_t bit) {
	char Command[128] = "";
	sprintf_s(Command, "MG @IN[%d];", bit);
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
	std::string* Response = new std::string(ReadBuffer);
	if (atoi(Response->c_str()) == 0) {
		bit = true;
	}
	else {
		bit = false;
	}
	return bit;
}


bool Galil::CheckSuccessfulWrite() {
	std::string* Response = new std::string(ReadBuffer);
	std::string convert = Response->c_str();
	std::size_t found = convert.find(':');
	if (found == std::string::npos) {
		return false;
	}
	else {
		return true;
	}
}


float Galil::AnalogInput(uint8_t channel) {
	float voltage = 0.0;
	char Command[128] = "";
	sprintf_s(Command, "MG @AN[%d];", channel);
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
	std::string* Response = new std::string(ReadBuffer);
	voltage = std::stof(Response->c_str());
	return voltage;
}


void Galil::AnalogOutput(uint8_t channel, double voltage) {
	char Command[128] = "";
	ReadBuffer[0] = { NULL };
	sprintf_s(Command, "AO %d,%f;", channel, voltage);
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
}


void Galil::AnalogInputRange(uint8_t channel, uint8_t range) {
	char Command[128] = "";
	sprintf_s(Command, "AQ %d,%d;", channel, range);
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
}


void Galil::WriteEncoder() {
	char Command[128] = "";
	sprintf_s(Command, "WE 0,0;");
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
}


int Galil::ReadEncoder() {
	int encoder = 0;
	char Command[128] = "";
	sprintf_s(Command, "QE;");
	Functions->GCommand(g, Command, ReadBuffer, sizeof(ReadBuffer), 0);
	std::string* Response = new std::string(ReadBuffer);
	encoder = std::stoi(Response->c_str());
	return encoder;
}


void Galil::setSetPoint(int s) {
	setPoint = s;
}


void Galil::setKp(double gain) {
	ControlParameters[0] = gain;
}


void Galil::setKi(double gain) {
	ControlParameters[1] = gain;
}


void Galil::setKd(double gain) {
	ControlParameters[2] = gain;
}


std::ostream& operator<<(std::ostream& output, Galil& galil) {
	GCon g = galil.g;
	char buf[1024];
	std::string* Version;
	std::string* Info;

	GVersion(buf, sizeof(buf));
	Version = new std::string(buf);

	GInfo(g, buf, sizeof(buf));
	Info = new std::string(buf);

	output << "GVersion Test: " << Version->c_str() << std::endl << std::endl << std::endl << "GInfo Test: " << Info->c_str() << std::endl << std::endl << std::endl;
	return output;
}