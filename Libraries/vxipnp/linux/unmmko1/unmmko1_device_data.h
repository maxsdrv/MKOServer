#ifndef UNMMKO1_DEVICE_DATA_H
#define UNMMKO1_DEVICE_DATA_H

#include <visa.h>

#include "un_threads.h"
#include "unmmko1.h"
#include <stddef.h>
#include "internal/un_threads.h"
#include "internal/un_queue.h"
#include "internal/unmmko1_constants.h"

#ifndef ViUInt64
#include <stdint.h>
#define ViUInt64 uint64_t
#endif

typedef enum {
    UNMMKO1_MODE_UNDEFINED          = 0x00,
    UNMMKO1_MODE_BUS_CONTROLLER     = 0x01,
    UNMMKO1_MODE_REMOTE_TERMINAL    = 0x02,
    UNMMKO1_MODE_BUS_MONITOR        = 0x03
} unmmko1_mode;

//! ��� �����������.
typedef enum {
    UNMMKO1_DIRECT                  = 0x0000,   //!< ���������������� �����������.
    UNMMKO1_TRANSFORM               = 0x0001    //!< ���������������� ����������� (�� ���������).
} unmmko1_connection_type;

//! �������������� ����������
typedef enum {
    UNMMKO1_USE_BUS_A_AND_B         = 0x0000,   //!< ��������� ����������� A � B (�� ���������)
    UNMMKO1_USE_ONLY_BUS_A          = 0x0002,   //!< ��������� ������ �� ���������� A
    UNMMKO1_USE_ONLY_BUS_B          = 0x0004,   //!< ��������� ������ �� ���������� B
    UNMMKO1_USE_MASK                = 0x0006    //!< ����� ������ �����������
} unmmko1_bus_usage;

typedef struct {
    ViUInt64 timestamp;
    ViUInt16 state;
    ViUInt16 value;
} milstd1553_word_s;

typedef enum {
    f_rubbish,                                  //!< ������� �� ������ ���������
    f_unknown                       = 0x0001,   //!< ������ ������ �� ��������
    // ������� �������� ��������� (1 - 6)
    f1                              = 0x0002,   //!< ��-��-��-...-��  <->  ��                     �������� ������ �� �� � ��
    f2                              = 0x0004,   //!< ��  <->  ��-��-��-...-��                     �������� ������ �� �� � ��
    f3                              = 0x0008,   //!< ��-��  <->  ��-��-��-...-��  <->  ��         �������� ������ �� �� � ��
    f4                              = 0x0010,   //!< ��  <->  ��                                  �������� ������� ����������
    f5                              = 0x0020,   //!< ��  <->  ��-��                               �������� ������� ���������� � ���� ����� ������
    f6                              = 0x0040,   //!< ��-��  <->  ��                               �������� ������� ���������� �� ������ ������
    // ������� ��������� ��������� (7 - 10)
    f7                              = 0x0080,   //!< ��-��-��-...-��                              ��������� �������� ������ �� �� �� ���� ��
    f8                              = 0x0100,   //!< ��-��  <->  ��-��-��-...-��                  ��������� �������� �� ������������ �� �� ���� ��������� ��
    f9                              = 0x0200,   //!< ��                                           ��������� �������� ������� ����������
    f10                             = 0x0400    //!< ��-��                                        ��������� �������� ������� ���������� �� ������ ������
} message_format_enum;

#pragma pack(push)
#pragma pack(1)

//! ����������� ������������������ ����
typedef struct {
    ViUInt64        timestamp;
    size_t          count;
    ViUInt64        types;
    ViUInt16        values[40];
    ViUInt16        errors;
    ViUInt64        gap;
} sequence_s;

#pragma pack(pop)

//! ����� ���������� ������ �� ���������
#define default_timeout_value_in_us     14

//! ����������� ������� ����
typedef struct {
    sequence_s sequence_a[3];                               //!< ������������������ ������� ��������� ���������� A
    sequence_s sequence_b[3];                               //!< ������������������ ������� ��������� ���������� B
    ViUInt64 last_a_timestamp;                              //!< ����� ���������� ��������� ���������� A
    ViUInt64 last_b_timestamp;                              //!< ����� ���������� ��������� ���������� B
    ViUInt64 current_timestamp;                             //!< ������� ����� ������� (������������ ��� ����������� ����� ����� ���������� �����)
    ViUInt64 timeout_value_in_us;                           //!< ����� ���������� ������
} accumulator_t;

//! ����� ������������ ������� ����
void reset_accumulator(accumulator_t* accumulator);

//! ������� ���������� ���������
typedef struct {
    ViUInt32 addresses_mask;                                //!< ����� ������� ��������� ���������, ������� �������� ���������������
    ViUInt32 receive_subaddresses[32];                      //!< ������� ����� ���������� ����� ��� ������� ���������� ����������
    ViUInt32 transmit_subaddresses[32];                     //!< ������� ����� ���������� �������� ��� ������� ���������� ����������
    ViUInt32 receive_modecodes[32];                         //!< ������� ����� ������ ���������� ����� ��� ������� ���������� ����������
    ViUInt32 transmit_modecodes[32];                        //!< ������� ����� ������ ���������� �������� ��� ������� ���������� ����������
} filter_rules_t;

//! ����� ������ ���������� ���������
void reset_filter_rules(filter_rules_t* filter_rules);

//! ��������� �������� ����
typedef struct {
    ViBoolean is_started;                                   //!< ���� ��������� �������
    int options;                                            //!< ����� �������� ��������
    accumulator_t accumulator;                              //!< ����������� ������� ���� ��������
    un_queue* messages;                                     //!< ������� ��������� ��������
    filter_rules_t filter_rules;                            //!< ������� ���������� ��������� ��������
} bus_monitor_settings_t;

void create_bus_monitor_settings(bus_monitor_settings_t* bus_monitor_settings);
void reset_bus_monitor_settings(bus_monitor_settings_t* bus_monitor_settings);
void destroy_bus_monitor_settings(bus_monitor_settings_t* bus_monitor_settings);

#define invalid_status_word 0xffffu

#pragma pack(push)
#pragma pack(1)

//! ��������� ���������� ����������
typedef struct {
    ViBoolean is_started;                                   //!< ���� ��������� �������
    int options;                                            //!< ����� �������� ���������� ����������

    struct {
        bool active;                                        //!< ���������� ���������� ����������
        ViUInt16 vector_word;                               //!< ����� ������ �� �� 16 "�������� ��������� �����"
        ViUInt16 selftest_word;                             //!< ����� ������ �� �� 19 "�������� ����� ��� ��"
        ViUInt16 status_word;                               //!< �������� �����
        struct {
            unmmko1_rt_subaddress_options options;          //!< ��������� ��������� ��������
            ViUInt16 data_words_count;                      //!< ���������� ���� ������
            ViUInt16* data_words;                           //!< ����� ������
        } subaddresses[31];
    } addresses[31];
} remote_terminal_settings_t;

#pragma pack(pop)

void create_remote_terminal_settings(remote_terminal_settings_t* remote_terminal_settings);
void reset_remote_terminal_settings(remote_terminal_settings_t* remote_terminal_settings);
void destroy_remote_terminal_settings(remote_terminal_settings_t* remote_terminal_settings);

//! ��������� ����������� ����
typedef struct {
    ViBoolean is_started;                                   //!< ���� ��������� �������
    int options;                                            //!< ����� �������� ����������� ����

    un_queue* schedule;                                     //!< ���������� �� ��������� ���������
    ViUInt16 repeat_count;                                  //!< ���������� ���������� ����������
} bus_controller_settings_t;

void create_bus_controller_settings(bus_controller_settings_t* bus_controller_settings);
void reset_bus_controller_settings(bus_controller_settings_t* bus_controller_settings);
void destroy_bus_controller_settings(bus_controller_settings_t* bus_controller_settings);

//! ��������� ������ ��������
typedef struct {
    ViUInt16 model_code;                                    //!< ��� ������ ��������
    ViSession init_session;                                 //!< ������, ���������� ��� ������������� ��������
    ViSession session;                                      //!< ������ ��������
    ViSession carrier_session;                              //!< ������ �������� ���������
    UNMMKO1_EVENT_HANDLER interrupt_request_handler;        //!< ���������������� ���������� ����������
    ViAddr interrupt_request_data;                          //!< ��������� �� ������, ������������ � ���������������� ���������� ����������
    ViUInt16 command_counter;                               //!< ������� ������

    unmmko1_mode current_mode;                              //!< ������� ����� ������ ��������
    bus_monitor_settings_t bus_monitor;                     //!< ��������� �������� ����
    remote_terminal_settings_t remote_terminal;             //!< ��������� ���������� ����������
    bus_controller_settings_t bus_controller;               //!< ��������� ����������� ����
} unmmko1_device_data;

//! \brief ������� ������� ��������� �� ������ �������� �� ��������� � VISA
unmmko1_device_data* get_device_data_from_visa(ViSession session);

//! \brief ������� ������� ��������� �� ������ �������� �� ��������� � �������� ���������
unmmko1_device_data* get_device_data_from_carrier(ViSession session);

//! \brief ������ ���������� � ������� ��������� �� ������ �������� �� ��������� � �������� ���������
//! ������ device_data ������ NULL, ���� ������ �������� � ��������� ������� �� ������� ��� �����������
#define get_device_data(session) \
    get_device_data_from_carrier(session); \
    if (is_not_valid_device_data(device_data)) device_data = NULL;

//! \brief ������ ���������� � ������� ��������� �� ������ �������� �� ��������� � �������� ���������
//! \returns ���������� ������ VI_ERROR_INV_SETUP, ���� ������ �������� � ��������� ������� �� ������� ��� �����������
#define get_device_data_or_error(session) \
    get_device_data_from_carrier(session); \
    if (is_not_valid_device_data(device_data)) return VI_ERROR_INV_SETUP

//! \brief ������� �������� �������������� ��������� �� ������ ��������
ViBoolean is_not_valid_device_data(unmmko1_device_data* data);

//! \brief �������� ��������� ������ ��������
//! \param[out] device_data ��������� �� ��������� �� ��������� ������ ��������
ViStatus create_device_data(unmmko1_device_data** device_data);

//! \brief ���������� ��������� ������ �������� � �������������� �������, ������������ � �������� ��� ���������� ����������
//! \param[in] device_data ��������� �� ��������� ������ �������� � �������� ������� ��������
ViStatus fill_device_data(unmmko1_device_data* device_data);

//! ����������� ��������� ������ ��������
//! \param[in] device_data ��������� �� ��������� ������ ��������
ViStatus destroy_device_data(unmmko1_device_data* device_data);

//! ����� ��������� ������ �������� � �������� ���
//! \param[in] device_data ��������� �� ��������� ������ ��������
ViStatus reset_device_data(unmmko1_device_data* device_data);

//! ���������� ���������� �� �������� ���������
ViBoolean _VI_FUNCH interrupt_request_handler(ViSession session, ViInt32 reason);

#endif // UNMMKO1_DEVICE_DATA_H
