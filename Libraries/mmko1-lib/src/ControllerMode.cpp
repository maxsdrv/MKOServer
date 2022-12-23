#include <iomanip>

#include "ControllerMode.h"
#include "MMKOErrors.h"
#include "MainBus.h"

ControllerMode::ControllerMode(MainBus* objectMmko, BUSLINE mkoLine)
		:
		commands(std::make_unique<unmmko1_command>()),
		busLine(mkoLine),
		controllerSession(objectMmko->getMkoSession())
{
	try {
		controllerStatus = MkoExceptions::CheckFunctions("CONTROLLER_CONFIGURE",
				controllerSession, unmmko1_bc_configure, controllerSession, UNMMKO1_BC_DEFAULT);
	}
	catch(const MkoExceptions& ex) {
		std::cerr << ex.what();
		objectMmko->reset(controllerSession);
	}
	std::cout << "ControllerMode()" << '\n';
}
ControllerMode::~ControllerMode()
{
	std::cout << "~ControllerMode()" << '\n';
}

uint16_t ControllerMode::PackCw(uint16_t address, uint16_t rxtx, uint16_t subAddress, uint16_t wordCount)
{
	return unmmko1_pack_cw(address, rxtx, subAddress, wordCount);
}

int32_t ControllerMode::BusToTerminalTransmit(uint16_t address, uint16_t subAddress, uint16_t wordCount,
		uint16_t* dataWords, RXTX type)
{
	auto commandWord = PackCw(address, static_cast<uint16_t>(type), subAddress, wordCount);
	*commands = unmmko1_bc_rt(static_cast<unmmko1_bus>(busLine), commandWord, dataWords);
	return unmmko1_bc_transmit_command(controllerSession, *commands);
}
int32_t ControllerMode::transmitCmdF1(uint16_t address, uint16_t subAddress, uint16_t wordCount,
		uint16_t* dataWords)
{
	*commands = unmmko1_f1(static_cast<unmmko1_bus>(busLine), address, subAddress, wordCount, dataWords);
	return unmmko1_bc_transmit_command(controllerSession, *commands);
}

void ControllerMode::StartController() const {
	std::cout << boost::format("CONTROLLER::STARTED %i") %controllerSession << '\n';
	unmmko1_bc_start(controllerSession);
}
void ControllerMode::StopController() const {
	std::cout << "CONTROLLER::STOPPED" << '\n';
	unmmko1_bc_stop(controllerSession);
}








