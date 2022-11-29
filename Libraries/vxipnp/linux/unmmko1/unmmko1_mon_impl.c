#include "unmmko1.h"
#include "internal/unmmko1_internal.h"
#include "internal/unmmko1_mon_utilities.h"
#include "unmbase.h"
#include <stdlib.h>

//! \brief ������� unmmko1_mon_configure ������������ ��������� �������� � ������ �������� ����.
//! ����� ������� ����� ������������ ����� �������� ��������.
//! ���� ������� ��� ������� � �����-���� ������ ������, �� ����� ����� ����������� ��������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \param mon_options ��������� �������� - ����������� ������ UNMMKO1_MON_* �� ������������ unmmko1_mon_options.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_configure(ViSession session, int mon_options) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode && device_data->bus_controller.is_started)
        unerror(unmmko1_bc_stop(session));
    else if (UNMMKO1_MODE_REMOTE_TERMINAL == device_data->current_mode && device_data->remote_terminal.is_started)
        unerror(unmmko1_rt_stop(session));
    else if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode && device_data->bus_monitor.is_started)
        unerror(unmmko1_mon_stop(session));

    unerror(unmmko1_mon_reset(session));
    unerror(set_current_mode(session, UNMMKO1_MODE_BUS_MONITOR));
    unerror(set_connection_type(session, (mon_options & UNMMKO1_MON_DIRECT) ? UNMMKO1_DIRECT : UNMMKO1_TRANSFORM));
    device_data->bus_monitor.options = mon_options;

Error:
    return status;
}

//! \brief ������� unmmko1_mon_reset ���������� ����� �������� ������� ���������� ������ � ����������, �������� ��������� ��������.
//! ����� ������� �� �������� � �������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_reset(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);
    reset_bus_monitor_settings(&device_data->bus_monitor);
    return status;
}

//! \brief ������� unmmko1_mon_set_timeout ������������� �������� ������� ���������� ������, ������������ ��������� ��� ������� ���������.
//! \param session ����� ������ ����� � ���������.
//! \param timeout_in_us �������� ������� ���������� ������ � �������������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_set_timeout(ViSession session, ViUInt16 timeout_in_us) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);
    device_data->bus_monitor.accumulator.timeout_value_in_us = timeout_in_us;
    return VI_SUCCESS;
}

//! \brief ������� unmmko1_mon_filter_address ��������� ���������� ������ �� ������� ��������� ���������.
//! ������ ������������ ������, �������� � ��������� addresses_mask.
//! ������� �������� ��� ��������� �������� �� ����� ���������� ���������� 0. ������� �������� ��� - �� ����� 31.
//! ���� ������� �� ������������, �� ���������������� ����� ��� ��������� ����������.
//! \param session ����� ������ ����� � ���������.
//! \param addresses_mask ����� ������� ��������� ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_filter_address(ViSession session, ViUInt32 addresses_mask) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);
    device_data->bus_monitor.filter_rules.addresses_mask = addresses_mask;
    return VI_SUCCESS;
}

//! \brief ������� unmmko1_mon_filter_subaddress ��������� ���������� ������ �� ���������� ��� ����������� ���������� ����������.
//! ������ ������������ ������� ���������� ����� � �������� ������, ������� ����� ������ ����� � ��������.
//! � ������ ����� ������� �������� ��� �������� �� �������� 0 ��� ��� ������� 0. ������� �������� ��� - �� �������� 31 ��� ��� ������� 31.
//! ���� ������� �� ������������, �� ���������������� ����� ��� ��������� ���������� ����������.
//! \param session ����� ������ ����� � ���������.
//! \param address ����� ���������� ����������.
//! \param receive_subaddresses ������� ����� ���������� �����.
//! \param transmit_subaddresses ������� ����� ���������� ��������.
//! \param receive_modecodes ������� ����� ������ ���������� �����.
//! \param transmit_modecodes ������� ����� ������ ���������� ��������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_filter_subaddress(ViSession session, ViUInt16 address, ViUInt32 receive_subaddresses, ViUInt32 transmit_subaddresses, ViUInt32 receive_modecodes, ViUInt32 transmit_modecodes) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_uint16(address, 0, 31))
        return VI_ERROR_PARAMETER2;

    device_data->bus_monitor.filter_rules.receive_subaddresses[address] = receive_subaddresses;
    device_data->bus_monitor.filter_rules.transmit_subaddresses[address] = transmit_subaddresses;
    device_data->bus_monitor.filter_rules.receive_modecodes[address] = receive_modecodes;
    device_data->bus_monitor.filter_rules.transmit_modecodes[address] = transmit_modecodes;

    return VI_SUCCESS;
}

//! \brief ������� unmmko1_mon_start ��������� ������� � ������ �������� ����.
//! ��������� �������� ���� �������������� ��������������� �������� unmmko1_mon_configure.
//! ������� ���� ����������� ������������ ��������� � �������� ����������� ���� ��� ���������� ����������, � � ���� ������ �������� unmmko1_mon_configure �� ����.
//! \remark ��� ������� �������� ���� ��������� ����� ���������� ���������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_start(ViSession session) {
    ViStatus status = VI_SUCCESS;
    ViUInt64 timeout_value_in_us = default_timeout_value_in_us;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_UNDEFINED == device_data->current_mode)
        unerror(unmmko1_mon_configure(session, UNMMKO1_MON_DEFAULT));

    if (device_data->bus_monitor.is_started)
        return UNMMKO1_WARNING_ALREADY_STARTED;

    // ����������� ������ ������
    unerror(unmbase_m_mode(session, UNMBASE_IOMODE_IN, UNMBASE_MODE_LOOP));

    // ��������� ������ ������
    unerror(unmbase_m_out16(session, REGISTER_FIFO, 0xFF));
    unerror(unmbase_m_start(session, UNMBASE_IOMODE_IN));

    // ������� ������ ������
    un_queue_clear(device_data->bus_monitor.messages);

    // ���������� �����������, �������� ��� ���� ��������� ������� ��������
    timeout_value_in_us = device_data->bus_monitor.accumulator.timeout_value_in_us;
    reset_accumulator(&device_data->bus_monitor.accumulator);
    device_data->bus_monitor.accumulator.timeout_value_in_us = timeout_value_in_us;

    // ��������� ������� ��� ��������������� �����, ���� ��� �� ���������� � �� ��������� ����������
    if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode)
        unerror(start_mon(session, device_data->bus_monitor.options & UNMMKO1_USE_MASK));

    device_data->bus_monitor.is_started = VI_TRUE;

Error:
    return status;
}

//! \brief ������� unmmko1_mon_status ���������� ��������� ������� �������� � ������ �������� ����.
//! \param session ����� ������ ����� � ���������.
//! \param status ��������� ������� �������� ���� (VI_TRUE - �������� ���� �������, VI_FALSE - ����������).
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_status(ViSession session, ViBoolean* status) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(status))
        return VI_ERROR_PARAMETER2;

    *status = device_data->bus_monitor.is_started;
    return VI_SUCCESS;
}

//! \brief ������� unmmko1_mon_stop ������������� �������, ���������� ����� � ������ �������� ����.
//! ���� ������� ���� ������������ ��������� � �������� ����������� ���� ��� ���������� ����������, �� ����� ������� ������� � ��������� ������ �������� ����.
//! ��������� ��������� ���� ��������� ����� ��������� � ������� ������� unmmko1_mon_messages_count � unmmko1_mon_messages_read.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_stop(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_UNDEFINED == device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (!device_data->bus_monitor.is_started)
        return UNMMKO1_WARNING_ALREADY_STOPPED;

    // ������������� ������ ������
    unerror(unmbase_m_async_stop(session, UNMBASE_IOMODE_IN));

    // ������������� ������� ��� ��������������� �����, ���� ��� �� ���������� � �� ��������� ����������
    if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode)
        unerror(stop_mon(session));
    device_data->bus_monitor.is_started = VI_FALSE;

Error:
    return status;
}

//! \brief ������� unmmko1_read_milstd1553_words ������������ ��� ������ ������ �� FIFO � �������������� �� � ����� ���.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus unmmko1_read_milstd1553_words(ViSession session, un_queue* words) {
    ViStatus status = VI_SUCCESS;
    ViUInt32 data_count = 0;
    size_t word_count = 0;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    // ����������� ���������� ������� ������
    unerror(unmbase_m_howmuch_data_q(session, UNMBASE_IOMODE_IN, &data_count));

    // ������ ����� �������� ������������ 64-������� �������, ������� ����������� ���������� ������ �� 2 32-������ �����
    word_count = data_count / 2;
    data_count = word_count * 2;

    // ���� ������ ����, �� ��������� �� � ���������� �� ���������
    if (data_count > 0) {
        // �������� ������ ��� ������
        ViUInt32* data = calloc(data_count, sizeof(ViUInt32));
        if (NULL == data)
            return 1;

        // ��������� ������
        unerror(unmbase_m_read_packet(session, data_count, data, &data_count));

        // ��������� ��������� ������ �� ������
        {
            size_t word_index = 0;
            milstd1553_word_s word;

            for (word_index = 0; word_index < word_count; ++word_index) {
                ViUInt32 w1 = data[2 * word_index];
                ViUInt32 w2 = data[2 * word_index + 1];

                word.value = (ViUInt16)(w1 & 0xffff);
                word.state = (ViUInt8)((w1 >> 16) & 0xff);
                word.timestamp = (((ViUInt64)w2) << 8) | (w1 >>24);

                un_queue_push_back(words, &word);
            }
        }

        // ����������� ������
        free(data);
    }

Error:
    return status;
}

//! \brief ������� unmmko1_mon_messages_count ������ ��� ������� ���������� ���������, ���������� ��������� ����.
//! ��������� ������������ ������� ��� � �������� ������ �������� � ������ �������� ���� (����� unmmko1_mon_start),
//! ��� � ����� �������� �������� (�������� unmmko1_mon_stop).
//! \param session ����� ������ ����� � ���������.
//! \param[out] messages_count ���������� ��������� ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_messages_count(ViSession session, ViUInt32* messages_count) {
    ViStatus status = VI_SUCCESS;
    un_queue* words = NULL;
    un_queue* messages = NULL;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(messages_count))
        return UNMMKO1_ERROR_INTERNAL;

    unerror(get_current_timestamp(session, &device_data->bus_monitor.accumulator.current_timestamp));

    words = un_queue_create(sizeof(milstd1553_word_s));
    unerror(unmmko1_read_milstd1553_words(session, words));

    messages = un_queue_create(sizeof(unmmko1_message));
    unmmko1_parse_words(words, &device_data->bus_monitor.accumulator, messages);
    unmmko1_filter_messages(messages, &device_data->bus_monitor.filter_rules, device_data->bus_monitor.messages);
    *messages_count = (ViUInt32)un_queue_size(device_data->bus_monitor.messages);

Error:
    un_queue_clear(words);
    un_queue_destroy(words);
    un_queue_clear(messages);
    un_queue_destroy(messages);
    return status;
}

//! \brief ������� unmmko1_mon_messages_read ������������ ������ ��������� � ������ ����������.
//! ��������� ������������ ������� ��� � �������� ������ �������� � ������ �������� ���� (����� unmmko1_mon_start),
//! ��� � ����� �������� �������� (�������� unmmko1_mon_stop).
//! \param session ����� ������ ����� � ���������.
//! \param[in] messages_count ���������� ���������, ������� ���������� �������.
//! \param[in,out] messages ������, � ������� ����������� ������� ���������. ������ ������ �������� ��� ������� �� messages_count ��������� ���� unmmko1_message.
//! \param[out] read_messages_count ���������� ��������� ��������� � messages.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_mon_messages_read(ViSession session, ViUInt32 messages_count, unmmko1_message* messages, ViUInt32* read_messages_count) {
    ViStatus status = VI_SUCCESS;
    unmmko1_message message;
    un_queue* words = NULL;
    un_queue* new_messages = NULL;
    ViUInt32 messages_index = 0;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(messages) || is_not_valid_pointer(read_messages_count))
        return UNMMKO1_ERROR_INTERNAL;

    unerror(get_current_timestamp(session, &device_data->bus_monitor.accumulator.current_timestamp));

    words = un_queue_create(sizeof(milstd1553_word_s));
    unerror(unmmko1_read_milstd1553_words(session, words));

    new_messages = un_queue_create(sizeof(unmmko1_message));
    unmmko1_parse_words(words, &device_data->bus_monitor.accumulator, new_messages);
    unmmko1_filter_messages(new_messages, &device_data->bus_monitor.filter_rules, device_data->bus_monitor.messages);

    *read_messages_count = MIN((ViUInt32)un_queue_size(device_data->bus_monitor.messages), messages_count);
    for (messages_index = 0; messages_index < *read_messages_count; ++messages_index) {
        un_queue_pop_first(device_data->bus_monitor.messages, &message);
        messages[messages_index] = message;
    }

Error:
    un_queue_clear(words);
    un_queue_destroy(words);
    un_queue_clear(new_messages);
    un_queue_destroy(new_messages);
    return status;
}
