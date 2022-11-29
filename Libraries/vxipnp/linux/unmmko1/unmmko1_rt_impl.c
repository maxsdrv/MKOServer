#include "unmmko1.h"
#include "internal/unmmko1_internal.h"
#include <stdlib.h>
#include <string.h>

//! \brief ������� unmmko1_rt_configure ������������ ��������� �������� � ������ ���������� ����������, ���������� ������ ����������� ��������� ���������.
//! ����� ������� ����� ������������ ����� �������� �����������.
//! ���� ������� ��� ������� � �����-���� ������ ������, �� ����� ����� ����������� ��������� ��������.
//!
//! ����� UNMMKO1_RT_BUS_A, UNMMKO1_RT_BUS_B � UNMMKO1_RT_BUS_A_AND_B (�� ���������) ���������� ����������, �� ������� �������� ����� ������� � ���������� ������������.
//! ��������� ����� UNMMKO1_RT_CUSTOM_RESPONSES ��������, ��� ��� ������� ����� � �������� ������ ��� ���� ���������� ��������� ��������� ���������� ��� ������������.
//! �� ������������ ������� �����/�������� ��������� ���������� �� ��������.
//! ��� ����, ����� ������� ������� ���������� (��� ����������� ������ � ���������) ����� ���������� ������ ������� ������� unmmko1_rt_set_subaddress_data.
//! ������������ ���� UNMMKO1_RT_CUSTOM_RESPONSES ����� � ��� �������, ����� ��������� ����� ������ �����/�������� ������� �������������.
//! ��������� �� ����� UNMMKO1_RT_DEFAULT_RESPONSES (�� ���������) ��������, ��� ��� ������� ����� � �������� ������ ��� ���� ���������� ����� �����������.
//! ����� �� ��������� ��� ����� ������ ����� ����������� � �������� 0, ������� ������� unmmko1_rt_set_subaddress_data �������� ���� ������ ����� ����������.
//! ���������� �� ����, ����� ���� (UNMMKO1_RT_CUSTOM_RESPONSES ��� UNMMKO1_RT_DEFAULT_RESPONSES) ����� ����������, ��������� ���������� ����� �������� �� ������� ����������.
//!
//! � ��������� ����������� ����������� ���������� ��������� ������ ����������: "�������� ��" (�� 2), "����������� ����������" (�� 4), "�������������� ����������" (�� 5),
//! "���������� �� � �������� ���������" (�� 8), "�������� ��������� �������" (�� 18).
//! \param session ����� ������ ����� � ���������.
//! \param addresses ����� ������� ��������� ��������� (������� �������� ��� �������� �� ����� ���������� ���������� 0, ������ ��� - �� 1 � ��� �����).
//! \param rt_options ��������� ���������� ���������� - ����������� ������ UNMMKO1_RT_* �� ������������ unmmko1_rt_options.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_configure(ViSession session, ViUInt32 addresses, int rt_options) {
    ViStatus status = VI_SUCCESS;
    ViUInt16 address = 0;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode && device_data->bus_controller.is_started)
        unerror(unmmko1_bc_stop(session));
    else if (UNMMKO1_MODE_REMOTE_TERMINAL == device_data->current_mode && device_data->remote_terminal.is_started)
        unerror(unmmko1_rt_stop(session));
    else if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode && device_data->bus_monitor.is_started)
        unerror(unmmko1_mon_stop(session));

    unerror(unmmko1_rt_reset(session));
    unerror(set_current_mode(session, UNMMKO1_MODE_REMOTE_TERMINAL));
    unerror(set_connection_type(session, (rt_options & UNMMKO1_RT_DIRECT) ? UNMMKO1_DIRECT : UNMMKO1_TRANSFORM));
    unerror(turn_rt(session, addresses));
    unerror(build_rt(session, addresses, (rt_options & UNMMKO1_RT_CUSTOM_RESPONSES) ? VI_FALSE : VI_TRUE));
    device_data->remote_terminal.options = rt_options;

    // �������� �������� ������ ��������� ��������� � ����� �������� ����� �� ���������
    for (address = 0; address < 31; ++address) {
        device_data->remote_terminal.addresses[address].active = (addresses & (1 << address)) ? true : false;
        device_data->remote_terminal.addresses[address].status_word = unmmko1_pack_sw(address, 0);
    }

    // ���� ������� ����� ���������� �������� ������� �� ���������, �� �������� �������� ������
    if (rt_options & UNMMKO1_RT_CUSTOM_RESPONSES) {
        for (address = 0; address < 31; ++address) {
            if (device_data->remote_terminal.addresses[address].active) {
                unerror(set_rt_configuration(session, address, VI_TRUE,
                                             device_data->remote_terminal.addresses[address].status_word,
                                             device_data->remote_terminal.addresses[address].vector_word,
                                             device_data->remote_terminal.addresses[address].selftest_word));
            }
        }
    }
    // ���� ������� ����� �������� ������� �� ���������, �� ��������� ���������� ������
    else {
        for (address = 0; address < 31; ++address) {
            if (!device_data->remote_terminal.addresses[address].active) {
                unerror(set_rt_configuration(session, address, VI_FALSE,
                                             device_data->remote_terminal.addresses[address].status_word,
                                             device_data->remote_terminal.addresses[address].vector_word,
                                             device_data->remote_terminal.addresses[address].selftest_word));
            }
        }
    }

Error:
    return status;
}

//! \brief ������� unmmko1_rt_reset ���������� ����� �������� � �������� �������� ���������.
//! ��������� ���������� ������������ �� �������� �� ��������� - UNMMKO1_RT_SUBADDRESS_DEFAULT.
//! �������� ���� ������, ����� ������ �� ������� ���������� ����������.
//! ��������� ��������� �������� �� ��������� - UNMMKO1_RT_RESPONSE_DEFAULT.
//! ���� ������� ��� ������� � �����-���� ������ ������, �� ����� ����� ����������� ��������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_reset(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode && device_data->bus_controller.is_started)
        unerror(unmmko1_bc_stop(session));
    else if (UNMMKO1_MODE_REMOTE_TERMINAL == device_data->current_mode && device_data->remote_terminal.is_started)
        unerror(unmmko1_rt_stop(session));
    else if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode && device_data->bus_monitor.is_started)
        unerror(unmmko1_mon_stop(session));

    reset_remote_terminal_settings(&device_data->remote_terminal);

Error:
    return status;
}

//! \brief ������� unmmko1_rt_set_status_word ������������� ��� ��������� �������� ��������� ����� ��� ����������� ������ ���������� ����������.
//! ����������� ����� ���������� ���������� ������ ���� ������� �������� �������� unmmko1_rt_configure.
//! ����� ������� �������� ����� � �������� ������ �������� � ������ ���������� ���������� (����� unmmko1_rt_start).
//! \param session ����� ������ ����� � ���������.
//! \param address ����� ���������� ����������.
//! \param status_word �������� ����� (����� ������� � ������� ������ ������� unmmko1_pack_sw).
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_set_status_word(ViSession session, ViUInt16 address, ViUInt16 status_word) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_REMOTE_TERMINAL != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (is_not_valid_uint16(address, 0, 30))
        return VI_ERROR_PARAMETER2;

    if (!device_data->remote_terminal.addresses[address].active)
        return UNMMKO1_ERROR_RT_ADDRESS_WAS_NOT_CONFIGURED;

    unerror(set_status_word(session, address, status_word));

Error:
    return status;
}

//! \brief ������� unmmko1_rt_set_subaddress_options ������������� ��� ��������� �������� ��������� ���������� ����������.
//! ����������� ����� ���������� ���������� ������ ���� ������� �������� �������� unmmko1_rt_configure.
//!
//! ���� UNMMKO1_RT_SUBADDRESS_DEFAULT ���������� ��������� �� ��������� - ��������� ���������� ������������ �������� 32 ���� ������ ��� ���������.
//! ������������� � ���� ������ ����� ������ (�������� unmmko1_rt_set_subaddress_data) ������������ ����������� ���� � ����� �� ������� �������� ������ (������ 2).
//! ���� � ������� �������� ������������� ������, ��� 32 ����� ������, �� ��������� ���������� ���������� ����� ������, ������� � ������ �������.
//! ���� UNMMKO1_RT_SUBADDRESS_WRAP ����������, ��� ��������� �������� ���������� ���������� ������ �������� � ������ ������������ �������� ������, �� ����
//! �����, �������� ��������� ����������� � ���� ��������, �������������� ����� ������ �� ��������.
//! � ���� ������ �������������� �������� 32 ���� ������, ���������� ���������� ����� ������, ������� � ������ �������.
//! ���� UNMMKO1_RT_SUBADDRESS_QUEUE �������� ����� ������� ������ ��� ���������� ���������.
//! � ���� ������ �������������� �������� ������������� ���������� ���� ������ (����� ������ � �� ���������� ��������������� �������� unmmko1_rt_set_subaddress_data �� ������ ��������).
//! ����������� ������ ������� ������ � ���, ��� ��������� ���������� ������� ����� ������ �� �������.
//! ������ ������ ������ �������� � �������� ����, ������� � ������� �����. ����������� ������� - ������� �� ���������� �� ��������� ���������� ������.
//!
//! ����� ������� ���������� � �������� ������ �������� � ������ ���������� ���������� (����� unmmko1_rt_start).
//! \param session ����� ������ ����� � ���������.
//! \param address ����� ���������� ����������.
//! \param subaddress �������� ���������� ����������.
//! \param subaddress_options ���� ��������� ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_set_subaddress_options(ViSession session, ViUInt16 address, ViUInt16 subaddress, unmmko1_rt_subaddress_options subaddress_options) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_REMOTE_TERMINAL != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (is_not_valid_uint16(address, 0, 30))
        return VI_ERROR_PARAMETER2;

    if (is_not_valid_uint16(subaddress, 1, 30))
        return VI_ERROR_PARAMETER3;

    if (subaddress_options != UNMMKO1_RT_SUBADDRESS_DEFAULT &&
        subaddress_options != UNMMKO1_RT_SUBADDRESS_WRAP &&
        subaddress_options != UNMMKO1_RT_SUBADDRESS_QUEUE)
        return VI_ERROR_PARAMETER4;

    if (!device_data->remote_terminal.addresses[address].active)
        return UNMMKO1_ERROR_RT_ADDRESS_WAS_NOT_CONFIGURED;

    if (device_data->remote_terminal.is_started)
        return UNMMKO1_ERROR_ILLEGAL_OPERATION_WHILE_RUNNING;

    if (device_data->remote_terminal.addresses[address].subaddresses[subaddress].options != subaddress_options) {
        unmmko1_rt_subaddress_options temp = device_data->remote_terminal.addresses[address].subaddresses[subaddress].options;
        device_data->remote_terminal.addresses[address].subaddresses[subaddress].options = subaddress_options;
        status = unmmko1_rt_set_subaddress_data(session, address, subaddress,
                                                device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words_count,
                                                device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words);
        if (0 != status)
            device_data->remote_terminal.addresses[address].subaddresses[subaddress].options = temp;
    }

    return status;
}

//! \brief ������� unmmko1_rt_set_subaddress_data ������������� ��� ��������� ���� ������ ��������� ���������� ����������.
//! ����� ������� ��� ��������� � ������ �� ��������� (UNMMKO1_RT_SUBADDRESS_DEFAULT) � � ������ ������������ �������� ������ (UNMMKO1_RT_SUBADDRESS_WRAP)
//! �������� � ���������� ���������� ���������� ���� ������. � ���� ������� ����� ������� �������� ����� � �������� ������ �������� (����� unmmko1_rt_start).
//! � ������ ������� ������ (UNMMKO1_RT_SUBADDRESS_QUEUE) ������� ��������� ������� �� data_words_count ���� ������ � ���������� � �� �����.
//! ����� ������� ���� ������ ���������� �������� ����������� ������� ������, ������� ���������� UNMMKO1_RT_QUEUE_SIZE_LIMIT ���� ������ �� ��� ������� ��������� ���������.
//! ����� ������� � ������ ������� ������ ���������� � �������� ������ �������� (����� unmmko1_rt_start).
//!
//! ���� ��������� ���������� ���� ��������� � ������ UNMMKO1_RT_CUSTOM_RESPONSES, �� unmmko1_rt_set_subaddress_data ������ ������� �����/�������� � ����� ��������� �����������.
//! \param session ����� ������ ����� � ���������.
//! \param address ����� ���������� ����������.
//! \param subaddress �������� ���������� ����������.
//! \param data_words_count ���������� ���� ������.
//! \param data_words ������ ���� ������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_set_subaddress_data(ViSession session, ViUInt16 address, ViUInt16 subaddress, int data_words_count, ViUInt16* data_words) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_REMOTE_TERMINAL != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (is_not_valid_uint16(address, 0, 30))
        return VI_ERROR_PARAMETER2;

    if (is_not_valid_uint16(subaddress, 1, 30))
        return VI_ERROR_PARAMETER3;

    if (0 == data_words_count)
        return VI_ERROR_PARAMETER4;

    if (NULL == data_words)
        return VI_ERROR_PARAMETER5;

    if (!device_data->remote_terminal.addresses[address].active)
        return UNMMKO1_ERROR_RT_ADDRESS_WAS_NOT_CONFIGURED;

    if (device_data->remote_terminal.is_started) {
        unmmko1_rt_subaddress_options subaddress_options = device_data->remote_terminal.addresses[address].subaddresses[subaddress].options;
        if (subaddress_options == UNMMKO1_RT_SUBADDRESS_QUEUE)
            return UNMMKO1_ERROR_CANNOT_UPDATE_DATA_WITH_QUEUE_MODE;

        // ��������� �������� � ��������� ����������
        unerror(update_rt_data(session, address, subaddress, (ViUInt16)data_words_count, data_words));
        // ��������� �������� � ��������� ���������� ����������
        memcpy(device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words, data_words, MIN((ViUInt16)data_words_count, 32) * sizeof(ViUInt16));
    }
    else {
        // ��������� �������� � ��������� ����������
        unmmko1_rt_subaddress_options subaddress_options = device_data->remote_terminal.addresses[address].subaddresses[subaddress].options;
        size_t packed_data_words_size = (size_t)(data_words_count + 1) / 2;
        ViUInt32* packed_data_words = (ViUInt32*)calloc(packed_data_words_size, sizeof(ViUInt32));
        memcpy(packed_data_words, data_words, (size_t)data_words_count * sizeof(ViUInt16));
        // �������� ���������� ���� ������ ��������� ��������� �� ������ �� ��������� ���������� ����������
        if (0 != data_words_count % 2 && data_words_count < device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words_count) {
            ViUInt32 align_word = (ViUInt32)device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words[data_words_count];
            packed_data_words[packed_data_words_size - 1] |= (align_word << 16);
        }

        // ��� ������� ����� ������ ������� ����� ���������� ������ 32 ����� ������
        if (subaddress_options != UNMMKO1_RT_SUBADDRESS_QUEUE)
            data_words_count = MIN(data_words_count, 32);

        unerror(set_rt_data(session, address, subaddress, subaddress_options, (ViUInt16)data_words_count, packed_data_words));

        // ��������� �������� � ��������� ���������� ����������
        if (subaddress_options == UNMMKO1_RT_SUBADDRESS_QUEUE) {
            free(device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words);
            device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words_count = (ViUInt16)data_words_count;
            device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words = (ViUInt16*)calloc((size_t)data_words_count, sizeof(ViUInt16));
            memcpy(device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words, data_words, (size_t)data_words_count * sizeof(ViUInt16));
        }
        else {
            memcpy(device_data->remote_terminal.addresses[address].subaddresses[subaddress].data_words, data_words, MIN((ViUInt16)data_words_count, 32) * sizeof(ViUInt16));
        }
    }

Error:
    return status;
}

//! \brief ������� unmmko1_rt_set_command_data ������������� ��� ��������� ���� ������ � ����� �� ������� ���������� � ������� ��������� 5.
//! ������������ ����� ������ �������� ���������� � ����� �� ������� ���������� "�������� ��������� �����" (�� 16) � "�������� ����� ��� ��" (�� 19).
//! ����� ������ � ������ ���������� ���������� �� ������� ���������� "�������� ��������� �������" (�� 18) ������������ ������ ��������� ��������.
//! ��������� ������������ ������� � �������� ������ �������� � ������ ���������� ���������� (����� unmmko1_rt_start).
//! \param session ����� ������ ����� � ���������.
//! \param address ����� ���������� ����������.
//! \param command_code ��� ������� ����������.
//! \param data_word ����� ������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_set_command_data(ViSession session, ViUInt16 address, ViUInt16 command_code, ViUInt16 data_word) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_REMOTE_TERMINAL != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (is_not_valid_uint16(address, 0, 30))
        return VI_ERROR_PARAMETER2;

    if (command_code != 16 && command_code != 19)
        return VI_ERROR_PARAMETER3;

    if (!device_data->remote_terminal.addresses[address].active)
        return UNMMKO1_ERROR_RT_ADDRESS_WAS_NOT_CONFIGURED;

    if (16 == command_code)
        device_data->remote_terminal.addresses[address].vector_word = data_word;
    else if (19 == command_code)
        device_data->remote_terminal.addresses[address].selftest_word = data_word;

    unerror(set_rt_configuration(session, address, VI_TRUE,
                                 device_data->remote_terminal.addresses[address].status_word,
                                 device_data->remote_terminal.addresses[address].vector_word,
                                 device_data->remote_terminal.addresses[address].selftest_word));

Error:
    return status;
}

//! \brief ������� unmmko1_rt_start ��������� ������� � ������ ���������� ����������.
//! ��������� ������ �������������� ��������������� �������� unmmko1_rt_configure.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_start(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_UNDEFINED == device_data->current_mode)
        unerror(unmmko1_rt_configure(session, 0x0000, UNMMKO1_RT_DEFAULT));

    if (UNMMKO1_MODE_REMOTE_TERMINAL != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (device_data->remote_terminal.is_started)
        return UNMMKO1_WARNING_ALREADY_STARTED;

    unerror(start_rt(session, device_data->remote_terminal.options & UNMMKO1_USE_MASK));
    device_data->remote_terminal.is_started = VI_TRUE;

Error:
    return status;
}

//! \brief ������� unmmko1_rt_status ���������� ��������� ������� �������� � ������ ���������� ����������.
//! \param session ����� ������ ����� � ���������.
//! \param status ��������� ������� ���������� ���������� (VI_TRUE - ����� ���������� ���������� �������, VI_FALSE - ����������).
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_status(ViSession session, ViBoolean* status) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(status))
        return VI_ERROR_PARAMETER2;

    *status = device_data->remote_terminal.is_started;
    return VI_SUCCESS;
}

//! \brief ������� unmmko1_rt_stop ������������� �������, ���������� ����� � ������ ���������� ����������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_rt_stop(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_REMOTE_TERMINAL != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (!device_data->remote_terminal.is_started)
        return UNMMKO1_WARNING_ALREADY_STOPPED;

    unerror(stop_rt(session));
    device_data->remote_terminal.is_started = VI_FALSE;

Error:
    return status;
}
