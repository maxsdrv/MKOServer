#pragma once


/** defines for error and closeDevice function */
#define ERROR "error"
#define CL_DV "close_device"

const int max_subaddress_data_size = 64;

enum class CARD_NUM
{
    CARD_1,
    CARD_2
};
enum class CORE_NUM
{
	CORE_1,
	CORE_2,
	CORE_MAX
};
enum class CHANNELS
{
	CHANNEL_1,
	CHANNEL_2,
	CH_COL
};
enum class F4_COMMAND
{
	BEGIN = -1,
	TAKE_COMMAND,
	SYNC,
	TRANSMIT_BW,
	START_SELFTEST,
	BLOCK_TRANSMITTER,
	DEBLOCK_TRANSMITTER,
	BLOCK_ERROR_FLAG,
	DEBLOCK_ERROR_FLAG,
	REINIT,
	RESERVE,/*********************/
	SEND_VECTOR_WORD = 16,
	SYNC_WITH_DW,
	SEND_LAST_CMD,
	SEND_VSK_WORD,
	BLOCK_Nth_TRANSMITTER,
	DEBLOCK_Nth_TRANSMITTER,
	END
};