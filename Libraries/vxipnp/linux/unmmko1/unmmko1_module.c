#include "unmmko1.h"

#include <stdlib.h>
#include <string.h>
#include <unmbase.h>
#include <stdio.h>
#include <time.h>
#include "internal/unmmko1_internal.h"

#define UNMMKO1_APP_VERSION "1.0.0"

//! \brief ������� unmmko1_init ���������� �������� �� ������������� ��������.
//! \param resource_name ������ ���������� ��������� ��� ���������� � ����� ���������� ��� �����������, � ������� ����������� �����.
//! \param idn_query ������ �������� ���������� ������������� ������� �������������� �������� � ��������� �������������.
//! \param do_reset ������ �������� ���������� ������������� ������ �������� � ��������� �������������.
//! \param session � ������ ���������� ������� ���������� ���������� ����� ������ ����� � ��������, ������� ���������� ��������� ��� ���� ����������� ������� ������� �������� ��������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_init(ViRsrc resource_name, ViBoolean idn_query, ViBoolean do_reset, ViSession* session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = NULL;
    ViSession temp_session = 0;
    ViSession defaultRM = 0;

    if (is_not_valid_boolean(idn_query))
        return VI_ERROR_PARAMETER2;
    if (is_not_valid_boolean(do_reset))
        return VI_ERROR_PARAMETER3;
    if (is_not_valid_pointer(session))
        return VI_ERROR_PARAMETER4;

    // ��������� ������ VISA
    unerror(viOpenDefaultRM(&defaultRM));

    // ��������� ������ ��������
    unerror(viOpen(defaultRM, resource_name, VI_NULL, VI_NULL, &temp_session));

    // ������ ������ ��������
    unerror(create_device_data(&device_data));

    // ��������� ��������� �� ���������� ������
    unerror(viSetAttribute(temp_session, VI_ATTR_USER_DATA, (ViAttrState)device_data));

    // ���������� ������
    device_data->session = 0;
    device_data->init_session = temp_session;
    *session = device_data->init_session;

    return status;

Error:
    if (is_valid_pointer(device_data))
        free(device_data);
    if (is_valid_session(temp_session))
        viClose(temp_session);
    if (is_valid_session(defaultRM))
        viClose(defaultRM);
    return status;
}

//! \brief ������� unmmko1_connect ������� � ������ ����� ��������, �������� ����� � ������� ������� �������������.
//! ������� ���������� �������� ���� ��� ����� ����� ������������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \param carrier_session ����� ������ ����� � ��������� ���������.
//! \param mezzanine_number ������� �������� �� �������� ���������.
//! \param idn_query ������ �������� ���������� ������������� ������� �������������� ��������.
//! \param do_reset ������ �������� ���������� ������������� ������ ��������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_connect(ViSession session, ViSession carrier_session, ViUInt16 mezzanine_number, ViBoolean idn_query, ViBoolean do_reset) {
    ViStatus status = VI_SUCCESS;
    ViSession temp_session = 0;
    unmmko1_device_data* device_data = get_device_data_from_visa(session);

    if (is_not_valid_device_data(device_data))
        return VI_ERROR_INV_SETUP;
    if (is_not_valid_session(carrier_session))
        return VI_ERROR_PARAMETER2;
    if (mezzanine_number < 1 || mezzanine_number > 8)
        return VI_ERROR_PARAMETER3;
    if (is_not_valid_boolean(idn_query))
        return VI_ERROR_PARAMETER4;
    if (is_not_valid_boolean(do_reset))
        return VI_ERROR_PARAMETER5;

    // �������� ������ ����� � �����
    if ((status = unmbase_m_init(carrier_session, mezzanine_number, &temp_session)) < 0)
        goto Fail;

    device_data->carrier_session = carrier_session;
    device_data->session = temp_session;

    // ������������� ����� � ��������� ���������
    unfail(unmbase_m_set_attribute(device_data->session, UNMBASE_MATTR_MEZ_EXTVI, (ViAttrState)session));

    // ������������� ����������� �����
    unfail(unmbase_m_set_attribute(session, UNMBASE_MATTR_ASYNC, (ViAttrState)1));

    // ��������� ��������� �� ���������� ������
    unfail(unmbase_m_set_attribute(session, UNMBASE_MATTR_USER_DATA, (ViAttrState)device_data));

    // ������ ID
    if (VI_TRUE == idn_query) {
        ViInt16 present = 0, idn_value = 0;
        unfail(unmbase_m_type_q(carrier_session, (ViInt16)mezzanine_number, &present, &idn_value));

        if ((idn_value & 0xFF) != UNMMKO1_MODEL_CODE)
            unfail(VI_ERROR_FAIL_ID_QUERY);
    }

    // ����������� ����/�����
    {
        // ����������� �������� ������ ������
        unfail(unmbase_m_first_delay(session, UNMBASE_IOMODE_IN,  M_FIRST_DELAY));
        unfail(unmbase_m_first_delay(session, UNMBASE_IOMODE_OUT, M_FIRST_DELAY));
        // ����������� ������� ��������� ��������
        unfail(unmbase_m_clock(session, UNMBASE_IOMODE_IN,  M_CLOCK));
        unfail(unmbase_m_clock(session, UNMBASE_IOMODE_OUT, M_CLOCK));
        // ����������� ������ �����
        unfail(unmbase_m_sample_width(session, UNMBASE_IOMODE_IN,  M_SAMPLE_WORDS));
        unfail(unmbase_m_sample_width(session, UNMBASE_IOMODE_OUT, M_SAMPLE_WORDS));

        // ������� - �������
        unfail(unmbase_m_trig_length(session, 1));

        // ����������� ������� ����� �� ���� � �����
        unfail(unmbase_m_mode(session, UNMBASE_IOMODE_IN,  UNMBASE_MODE_BLOCK));
        unfail(unmbase_m_mode(session, UNMBASE_IOMODE_OUT, UNMBASE_MODE_BLOCK));
    }

    // �������� ������ ��������, �������� ������ ������� ����� ����� ����������
    {
        ViInt32 mezzanine, mezzanine_count;
        ViUInt32 memory_size, memory_chunk;

        unfail(unmbase_mem_q(carrier_session, &memory_size));
        if (0 == memory_size)
            unfail(UNMMKO1_ERROR_NOT_ENOUGH_MEMORY);

        mezzanine_count = 0;
        for (mezzanine = 1; mezzanine <= 4; ++mezzanine) {
            ViInt16 present = 0, mtype = 0;
            unmbase_m_type_q(carrier_session, (ViInt16)mezzanine, &present, &mtype);
            if (present)
                mezzanine_count++;
        }

        // ��������� ������ ������
        memory_size /= (ViUInt32)mezzanine_count;

        // ������� ���������� ������� ������
        memory_chunk = (ViUInt32)(~0xFF) & (memory_size / 16);

        // ������� ��������� ������, ���� ���� ������
        unfail(unmbase_m_alloc(session, UNMBASE_IOMODE_IN,  0, VI_NULL));
        unfail(unmbase_m_alloc(session, UNMBASE_IOMODE_OUT, 0, VI_NULL));

        memory_size += memory_chunk;
        do {
            ViUInt32 addr;
            memory_size -= memory_chunk;
            if (memory_size <= 0)
                unfail(UNMMKO1_ERROR_NOT_ENOUGH_MEMORY);

            status = unmbase_m_alloc(session, UNMBASE_IOMODE_IN, memory_size / 2, &addr);
            if (status >= 0)
                status = unmbase_m_alloc(session, UNMBASE_IOMODE_OUT, memory_size / 2, &addr);
        } while (status && memory_chunk);

        unfail(status);
    }

    {
        // �����
        unmmko1_reset(session);

        // ���������� �������� ������ � ������� ��������
        unmbase_m_out16(session, REGISTER_FIFO, 0xFF);
        unmbase_m_start(session, UNMBASE_IOMODE_BOTH);
        unmbase_m_stop(session,  UNMBASE_IOMODE_BOTH);

        // ��������� �����
        unmmko1_reset(session);
    }

    // ��������� ������ ��������
    unfail(fill_device_data(device_data));

    // ������������� ���������� ������� �� �������� ��������
    unfail(unmbase_m_handler(session, (ViAddr)(&interrupt_request_handler)));

    // ��������� ����������
    unfail(unmbase_m_config_event(session, VI_ON));

    return status;

Fail:
    if (is_valid_session(session))
        unmbase_m_close(session);
    return status;
}

//! \brief ������� unmmko1_self_test ���������� ������������ �������� � ���������� ��� ���������.
//! \param session ����� ������ ����� � ���������.
//! \param result ��� ���������� ������������.
//! \param message ��������� � ���������� ������������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_self_test(ViSession session, ViInt16* result, ViChar message[]) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(result))
        return VI_ERROR_PARAMETER2;
    if (is_not_valid_pointer(message))
        return VI_ERROR_PARAMETER3;

    // ������������� ������� � ���������� ��� ���������
    unerror(unmmko1_reset(session));

    // ��������� ��������� ������������
    status = self_test(session);
    *result = (ViInt16)status;
    if (0 == status)
        strcpy(message, "������������ ������� �������.");
    else if (-1 == status)
        strcpy(message, "������ FIFO.");
    else if (-2 == status)
        strcpy(message, "������ ���� ������.");
    else if (-3 == status)
        strcpy(message, "������ ������.");
    else {
        unmmko1_error_message(session, status, message);
        *result = -1;
    }

    // ���������� ��������� ��������
    unmmko1_reset(session);
    return (0 == *result ? VI_SUCCESS : UNMMKO1_ERROR_SELFTEST);

Error:
    *result = (ViInt16)status;
    unmmko1_error_message(session, status, message);
    unmmko1_reset(session);
    return status;
}

ViBoolean wait_for_monitoring_one_message(ViSession session, unmmko1_message* message) {
    ViUInt32 message_count = 0;
    clock_t start_time = 0, elapsed_time = 0;
    const clock_t timeout = 5 * CLOCKS_PER_SEC;

    start_time = clock();
    do {
        unmmko1_mon_messages_count(session, &message_count);
        if (message_count > 0)
            break;

        un_sleep(5);
        elapsed_time = clock() - start_time;
    } while (0 == message_count && elapsed_time < timeout);

    if (message_count > 0)
        unmmko1_mon_messages_read(session, 1, message, &message_count);

    return (message_count > 0);
}

ViBoolean compare_messages(unmmko1_message* message, ViUInt16 activities, ViUInt16 errors, unmmko1_command command,
                           ViUInt16 response_1_status_word, ViUInt16 response_1_data_words_count, ViUInt16 response_1_data_words[32],
ViUInt16 response_2_status_word, ViUInt16 response_2_data_words_count, ViUInt16 response_2_data_words[32]) {
    if (message->activity != activities)
        return VI_FALSE;
    if (message->error != errors)
        return VI_FALSE;
    if (message->command.command_word_1 != command.command_word_1)
        return VI_FALSE;
    if (message->command.command_word_2 != command.command_word_2)
        return VI_FALSE;
    if (message->command.data_words_count != command.data_words_count)
        return VI_FALSE;
    if (memcmp(message->command.data_words, command.data_words, sizeof(message->command.data_words)))
        return VI_FALSE;
    if (message->response_1.status_word != response_1_status_word)
        return VI_FALSE;
    if (message->response_1.data_words_count != response_1_data_words_count)
        return VI_FALSE;
    if (memcmp(message->response_1.data_words, response_1_data_words, sizeof(message->response_1.data_words)))
        return VI_FALSE;
    if (message->response_2.status_word != response_2_status_word)
        return VI_FALSE;
    if (message->response_2.data_words_count != response_2_data_words_count)
        return VI_FALSE;
    if (memcmp(message->response_2.data_words, response_2_data_words, sizeof(message->response_2.data_words)))
        return VI_FALSE;

    return VI_TRUE;
}

//! \brief ������� unmmko1_test_exchange ���������� ������������ ������ � ��������� ������� ������ ��������.
//! ������������ ������ - ���������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \param result ��� ���������� ������������.
//! \param message ��������� � ���������� ������������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_test_exchange(ViSession session, ViInt16* result, ViChar message[]) {
    ViStatus status = VI_SUCCESS;
    unmmko1_message m;
    ViUInt16 data_words[32] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32};
    ViUInt16 zero_data_words[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ViUInt16 one_data_words[32] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    ViUInt16 data_words_queue[256];
    ViUInt16 index;
    ViBoolean message_received = VI_FALSE;

    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(result))
        return VI_ERROR_PARAMETER2;
    if (is_not_valid_pointer(message))
        return VI_ERROR_PARAMETER3;

    // ������������� ������� � ���������� ��� ���������
    unerror(unmmko1_reset(session));

    // ��������� ��������� ������������
    // ����� ��������
    {
        unmmko1_command command_f1 = unmmko1_f1(UNMMKO1_BUS_A, RT_1, SA_2, 32, data_words);

        // ��������������� �� ���������� A
        unerror(unmmko1_mon_configure(session, UNMMKO1_MON_DIRECT | UNMMKO1_MON_BUS_A));
        unerror(unmmko1_mon_start(session));
        command_f1.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
        unerror(unmmko1_bc_transmit_command(session, command_f1));
        message_received = wait_for_monitoring_one_message(session, &m);
        unerror(unmmko1_mon_stop(session));
        if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1, UNMMKO1_MSG_ERR_NO_RESPONSE | UNMMKO1_MSG_ERR_PROTOCOL, command_f1,
                              0, 0, zero_data_words, 0, 0, zero_data_words))
            fail_test(0x0001, "������ �������� (���������� A)");

        // ��������������� �� ���������� B
        unerror(unmmko1_mon_configure(session, UNMMKO1_MON_DIRECT | UNMMKO1_MON_BUS_B));
        unerror(unmmko1_mon_start(session));
        command_f1.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
        unerror(unmmko1_bc_transmit_command(session, command_f1));
        message_received = wait_for_monitoring_one_message(session, &m);
        unerror(unmmko1_mon_stop(session));
        if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1, UNMMKO1_MSG_ERR_NO_RESPONSE | UNMMKO1_MSG_ERR_PROTOCOL, command_f1,
                              0, 0, zero_data_words, 0, 0, zero_data_words))
            fail_test(0x0002, "������ �������� (���������� B)");
    }

    // ����� ���������� ����������
    {
        // ������ ��������� 1
        {
            unmmko1_command command_f1 = unmmko1_f1(UNMMKO1_BUS_A, RT_1, SA_2, 32, data_words);

            // ���������� A
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_A | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f1.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f1));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f1,
                                  unmmko1_pack_sw(RT_1, 0), 0, zero_data_words, 0, 0, zero_data_words))
                fail_test(0x0003, "������ ���������� ���������� (���������� A): ������ ��������� 1");

            // ���������� B
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_B | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f1.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f1));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f1,
                                  unmmko1_pack_sw(RT_1, 0), 0, zero_data_words, 0, 0, zero_data_words))
                fail_test(0x0004, "������ ���������� ���������� (���������� B): ������ ��������� 1");
        }

        // ������ ��������� 2
        {
            unmmko1_command command_f2 = unmmko1_f2(UNMMKO1_BUS_B, RT_1, SA_2, 32);

            // ���������� A
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_A | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_set_subaddress_data(session, RT_1, SA_2, 32, data_words));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f2.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f2));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f2,
                                  unmmko1_pack_sw(RT_1, 0), 32, data_words, 0, 0, zero_data_words))
                fail_test(0x0005, "������ ���������� ���������� (���������� A): ������ ��������� 2");

            // ���������� B
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_B | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_set_subaddress_data(session, RT_1, SA_2, 32, data_words));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f2.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f2));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f2,
                                  unmmko1_pack_sw(RT_1, 0), 32, data_words, 0, 0, zero_data_words))
                fail_test(0x0006, "������ ���������� ���������� (���������� B): ������ ��������� 2");
        }

        // ������ ��������� 2 � ������ �������
        {
            unmmko1_command command_f2 = unmmko1_f2(UNMMKO1_BUS_B, RT_1, SA_2, 32);
            for (index = 0; index < 256; ++index)
                data_words_queue[index] = index;

            // ���������� A
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_A | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_set_subaddress_options(session, RT_1, SA_2, UNMMKO1_RT_SUBADDRESS_QUEUE));
            unerror(unmmko1_rt_set_subaddress_data(session, RT_1, SA_2, 256, data_words_queue));

            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f2.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
            for (index = 0; index < 256 / 32; ++index) {
                unerror(unmmko1_bc_transmit_command(session, command_f2));
                un_sleep(10);
            }
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));

            for (index = 0; index < 256 / 32; ++index) {
                message_received = wait_for_monitoring_one_message(session, &m);
                if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f2,
                                      unmmko1_pack_sw(RT_1, 0), 32, data_words_queue + index * 32, 0, 0, zero_data_words))
                    fail_test(0x0007, "������ ���������� ���������� (���������� A): ������ ��������� 2 � ������ �������");
            }

            // ���������� B
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_B | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_set_subaddress_options(session, RT_1, SA_2, UNMMKO1_RT_SUBADDRESS_QUEUE));
            unerror(unmmko1_rt_set_subaddress_data(session, RT_1, SA_2, 256, data_words_queue));

            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f2.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
            for (index = 0; index < 256 / 32; ++index) {
                unerror(unmmko1_bc_transmit_command(session, command_f2));
                un_sleep(10);
            }
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));

            for (index = 0; index < 256 / 32; ++index) {
                message_received = wait_for_monitoring_one_message(session, &m);
                if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f2,
                                      unmmko1_pack_sw(RT_1, 0), 32, data_words_queue + index * 32, 0, 0, zero_data_words))
                    fail_test(0x0008, "������ ���������� ���������� (���������� B): ������ ��������� 2 � ������ �������");
            }
        }

        // ������ ��������� 4
        {
            unmmko1_command command_f4 = unmmko1_f4(UNMMKO1_BUS_B, RT_1, SA_MC0, 1);

            // ���������� A
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_A | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f4.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f4));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f4,
                                  unmmko1_pack_sw(RT_1, 0), 0, zero_data_words, 0, 0, zero_data_words))
                fail_test(0x0009, "������ ���������� ���������� (���������� A): ������ ��������� 4");

            // ���������� B
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_B | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f4.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f4));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f4,
                                  unmmko1_pack_sw(RT_1, 0), 0, zero_data_words, 0, 0, zero_data_words))
                fail_test(0x000a, "������ ���������� ���������� (���������� B): ������ ��������� 4");
        }

        // ������ ��������� 5
        {
            ViUInt16 data_word = 0x5432u;
            unmmko1_command command_f5 = unmmko1_f5(UNMMKO1_BUS_B, RT_1, SA_MC0, 16);

            // ���������� A
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_A | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_set_command_data(session, RT_1, 16, data_word));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f5.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f5));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            one_data_words[0] = data_word;
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f5,
                                  unmmko1_pack_sw(RT_1, 0), 1, one_data_words, 0, 0, zero_data_words))
                fail_test(0x000b, "������ ���������� ���������� (���������� A): ������ ��������� 5");

            // ���������� B
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_B | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_set_command_data(session, RT_1, 16, data_word));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f5.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f5));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f5,
                                  unmmko1_pack_sw(RT_1, 0), 1, one_data_words, 0, 0, zero_data_words))
                fail_test(0x000c, "������ ���������� ���������� (���������� B): ������ ��������� 5");
        }

        // ������ ��������� 6
        {
            unmmko1_command command_f6 = unmmko1_f6(UNMMKO1_BUS_B, RT_1, SA_MC0, 17, 0);

            // ���������� A
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_A | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f6.activity = UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f6));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f6,
                                  unmmko1_pack_sw(RT_1, 0), 0, zero_data_words, 0, 0, zero_data_words))
                fail_test(0x000d, "������ ���������� ���������� (���������� A): ������ ��������� 6");

            // ���������� B
            unerror(unmmko1_rt_configure(session, 1 << RT_1, UNMMKO1_RT_DIRECT | UNMMKO1_RT_BUS_B | UNMMKO1_RT_DEFAULT_RESPONSES));
            unerror(unmmko1_rt_start(session));
            unerror(unmmko1_mon_start(session));
            command_f6.activity = UNMMKO1_MSG_ACT_BUS_A | UNMMKO1_MSG_ACT_CWD_1;
            unerror(unmmko1_bc_transmit_command(session, command_f6));
            message_received = wait_for_monitoring_one_message(session, &m);
            unerror(unmmko1_mon_stop(session));
            unerror(unmmko1_rt_stop(session));
            if (!message_received || !compare_messages(&m, UNMMKO1_MSG_ACT_BUS_B | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_SWD_1, UNMMKO1_MSG_ERR_OK, command_f6,
                                  unmmko1_pack_sw(RT_1, 0), 0, zero_data_words, 0, 0, zero_data_words))
                fail_test(0x000e, "������ ���������� ���������� (���������� B): ������ ��������� 6");
        }
    }

    *result = 0;
    strcpy(message, "���� ������ ������� ��� ������");

Fail:
    unmmko1_reset(session);
    return status;

Error:
    *result = (ViInt16)status;
    unmmko1_error_message(session, status, message);
    unmmko1_reset(session);
    return status;
}

//! \brief ������� unmmko1_test_memory ���������� ������������ ������ ��������.
//! ������������ ������ - ���������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \param result ��� ���������� ������������.
//! \param message ��������� � ���������� ������������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_test_memory(ViSession session, ViInt16* result, ViChar message[]) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(result))
        return VI_ERROR_PARAMETER2;
    if (is_not_valid_pointer(message))
        return VI_ERROR_PARAMETER3;

    // ������������� ������� � ���������� ��� ���������
    unerror(unmmko1_reset(session));

    // ������������� ��� ��������� ����������, ���������� � ������ �������� ���������
    {
        ViUInt16 address = 0;
        unerror(unmmko1_rt_configure(session, 0xffffffff, UNMMKO1_RT_DEFAULT));
        for (address = 0; address < 31; ++address)
            unerror(set_rt_configuration(session, address, VI_TRUE, address, address + 1, address + 2));
        for (address = 0; address < 31; ++address) {
            ViBoolean on = VI_FALSE;
            ViUInt16 status_word = 0, vector_word = 0, selftest_word = 0;
            unerror(get_rt_configuration(session, address, &on, &status_word, &vector_word, &selftest_word));
            if (!on || status_word != address || vector_word != address + 1 || selftest_word != address + 2)
                fail_test(0x0001, "������ ������ ������������ ���������� ����������");
        }
    }

    // ���������� ������ � ������ UNMMKO1_RT_SUBADDRESS_DEFAULT � ������ ��
    {
#define DATA_WORDS_COUNT 32
        ViUInt16 data_words[DATA_WORDS_COUNT];
        ViUInt16 address = 0, subaddress = 0, data_index = 0;
        for (address = 0; address < 31; ++address) {
            for (subaddress = 1; subaddress < 31; ++subaddress) {
                for (data_index = 0; data_index < DATA_WORDS_COUNT; ++data_index)
                    data_words[data_index] = (address * 32 + subaddress) * 32 + data_index;
                unerror(unmmko1_rt_set_subaddress_data(session, address, subaddress, DATA_WORDS_COUNT, data_words));
            }
        }

        for (address = 0; address < 31; ++address) {
            for (subaddress = 1; subaddress < 31; ++subaddress) {
                unmmko1_rt_subaddress_options options;
                ViUInt16 data_words_count;
                ViUInt32 packed_data_word[DATA_WORDS_COUNT / 2];
                for (data_index = 0; data_index < DATA_WORDS_COUNT; ++data_index)
                    data_words[data_index] = (address * 32 + subaddress) * 32 + data_index;
                unerror(get_rt_data(session, address, subaddress, &options, &data_words_count, packed_data_word));
                if (options != UNMMKO1_RT_SUBADDRESS_DEFAULT || DATA_WORDS_COUNT != data_words_count || memcmp(data_words, packed_data_word, sizeof(packed_data_word)))
                    fail_test(0x0002, "������ ������ ������ ���������� ����������");
            }
        }
    }

    // ���������� ������ � ������ UNMMKO1_RT_SUBADDRESS_QUEUE � ������ ��
    {
#undef DATA_WORDS_COUNT
#define DATA_WORDS_COUNT 256
        ViUInt16 data_words[DATA_WORDS_COUNT];
        ViUInt16 address = 0, subaddress = 0, data_index = 0;
        for (address = 0; address < 31; ++address) {
            for (subaddress = 1; subaddress < 31; ++subaddress) {
                for (data_index = 0; data_index < DATA_WORDS_COUNT; ++data_index)
                    data_words[data_index] = (address * 32 + subaddress) * 32 + data_index;
                unerror(unmmko1_rt_set_subaddress_options(session, address, subaddress, UNMMKO1_RT_SUBADDRESS_QUEUE));
                unerror(unmmko1_rt_set_subaddress_data(session, address, subaddress, DATA_WORDS_COUNT, data_words));
            }
        }

        for (address = 0; address < 31; ++address) {
            for (subaddress = 1; subaddress < 31; ++subaddress) {
                unmmko1_rt_subaddress_options options;
                ViUInt16 data_words_count;
                ViUInt32 packed_data_word[DATA_WORDS_COUNT / 2];
                for (data_index = 0; data_index < DATA_WORDS_COUNT; ++data_index)
                    data_words[data_index] = (address * 32 + subaddress) * 32 + data_index;
                unerror(get_rt_data(session, address, subaddress, &options, &data_words_count, packed_data_word));
                if (options != UNMMKO1_RT_SUBADDRESS_QUEUE || DATA_WORDS_COUNT != data_words_count || memcmp(data_words, packed_data_word, sizeof(packed_data_word)))
                    fail_test(0x0003, "������ ������ ����������� ������ ���������� ����������");
            }
        }
#undef DATA_WORDS_COUNT
    }

    *result = 0;
    strcpy(message, "���� ������ ������� ��� ������");

Fail:
    unmmko1_reset(session);
    return status;

Error:
    *result = (ViInt16)status;
    unmmko1_error_message(session, status, message);
    unmmko1_reset(session);
    return status;
}

//! \brief ������� unmmko1_reset ������� ���������� ����� �������� � ��������� �� ���������.
//! ���� ������� ��� ������� � �����-���� ������ ������, �� ����� ����� ����������� ��������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_reset(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    // ������������� �������, ���� �� �������
    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode && device_data->bus_controller.is_started)
        unerror(unmmko1_bc_stop(session));
    else if (UNMMKO1_MODE_REMOTE_TERMINAL == device_data->current_mode && device_data->remote_terminal.is_started)
        unerror(unmmko1_rt_stop(session));
    else if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode && device_data->bus_monitor.is_started)
        unerror(unmmko1_mon_stop(session));

    // ��������� ����������
    unerror(unmbase_m_config_event(session, VI_OFF));

    // ������������� � ���������� �������
    unmbase_m_stop(session,  UNMBASE_IOMODE_BOTH);
    unmbase_m_reset(session, UNMBASE_IOMODE_BOTH);

    // ���������� ������ ��������
    unerror(reset_device_data(device_data));

    // ����������� �������� ������ ������
    unerror(unmbase_m_first_delay(session, UNMBASE_IOMODE_IN,  M_FIRST_DELAY));
    unerror(unmbase_m_first_delay(session, UNMBASE_IOMODE_OUT, M_FIRST_DELAY));
    // ����������� ������� ��������� ��������
    unerror(unmbase_m_clock(session, UNMBASE_IOMODE_IN,  M_CLOCK));
    unerror(unmbase_m_clock(session, UNMBASE_IOMODE_OUT, M_CLOCK));
    // ����������� ������ �����
    unerror(unmbase_m_sample_width(session, UNMBASE_IOMODE_IN,  M_SAMPLE_WORDS));
    unerror(unmbase_m_sample_width(session, UNMBASE_IOMODE_OUT, M_SAMPLE_WORDS));
    // ����������� ����� �����
    unerror(unmbase_m_block_size(session, UNMBASE_IOMODE_IN,  512 / M_SAMPLE_BYTES));
    unerror(unmbase_m_block_size(session, UNMBASE_IOMODE_OUT, 512 / M_SAMPLE_BYTES));

    // ������� - �������
    unerror(unmbase_m_trig_length(session, 1));

    // ����������� ������� ����� �� ���� � �����
    unerror(unmbase_m_mode(session, UNMBASE_IOMODE_IN,  UNMBASE_MODE_BLOCK));
    unerror(unmbase_m_mode(session, UNMBASE_IOMODE_OUT, UNMBASE_MODE_BLOCK));

    // ������ ������
    unerror(unmbase_m_packet_size(session, UNMBASE_IOMODE_IN, 0));

    // ��������� ����������
    unerror(unmbase_m_config_event(session, VI_ON));

Error:
    return status;
}

//! \brief ������� unmmko1_install_interrupt_handler ������������� ���������������� ������� ��� ����� ���������� �� ��������.
//! \param session ����� ������ ����� � ���������.
//! \param handler ���������������� ���������� ���������� - ��������� �� ������� � ����������, �������� � UNMMKO1_EVENT_HANDLER.
//! \param data ��������� �� ������, ������������ � ���������������� ���������� ����������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_install_interrupt_handler(ViSession session, UNMMKO1_EVENT_HANDLER handler, ViAddr data) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    device_data->interrupt_request_handler = handler;
    device_data->interrupt_request_data = data;

    return VI_SUCCESS;
}

//! \brief ������� unmmko1_error_query ���������� ������� ��� � ��������� �� ������ ��� ������ � ���������.
//! ��� ������� �������� ������� �� �������������� � ���������� ������ VI_WARN_NSUP_ERROR_QUERY.
//! \param session ����� ������ ����� � ���������.
//! \param error ��� ������.
//! \param message ��������� �� ������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_error_query(ViSession session, ViPInt32 error, ViChar message[]) {
    (void)session;
    (void)error;
    (void)message;
    return VI_WARN_NSUP_ERROR_QUERY;
}

//! \brief ������� unmmko1_revision_query ���������� ������ �������� �����������.
//! ��� ������� �������� ������� �� ������������ ������� ������ ����������� ����������� � ���������� ������ VI_WARN_NSUP_REV_QUERY.
//! \param session ����� ������ ����� � ���������.
//! \param software_version ������ ������������ ����������� ��������.
//! \param hardware_version ������ ����������� ����������� �������� (�� ��������������).
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_revision_query(ViSession session, ViChar software_version[], ViChar hardware_version[]) {
    (void)session;
    strcpy(software_version, UNMMKO1_APP_VERSION);
    strcpy(hardware_version, "<not available>");
    return VI_WARN_NSUP_REV_QUERY;
}

//! \brief ������� unmmko1_error_message �������� � �������������� ��� ���������, ������������ �����-���� �������� �������� � ���������� ������ ��������� �� ������.
//! �������������� ������� ��������� �� ���������� � ������� ������, ���� ������� �� ������� ������������ ���������� (������ LC_CTYPE).
//! \param session ����� ������ ����� � ���������.
//! \param status ��� ������.
//! \param error_message ��������� �� ������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_error_message(ViSession session, ViStatus status, ViChar error_message[]) {
    (void)session;

    if ((status >= UNMMKO1_WARNING_FIRST_CODE && status <= UNMMKO1_WARNING_LAST_CODE) ||
        (status >= UNMMKO1_ERROR_FIRST_CODE && status <= UNMMKO1_ERROR_LAST_CODE)) {
        get_localized_error_message(status, error_message);
    }
    else if (unmbase_error_message(VI_NULL, status, error_message) < 0) {
        strcpy(error_message, "Unknown status");
        return VI_WARN_UNKNOWN_STATUS;
    }

    return VI_SUCCESS;
}

//! \brief ������� unmmko1_close ��������� ����� ����� � ���������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_close(ViSession session) {
    ViStatus status = VI_SUCCESS;
    ViSession init_session = 0;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    unerror(unmmko1_reset(session));

    init_session = device_data->init_session;
    destroy_device_data(device_data);

    // ��������� ������ ��������
    if (is_valid_session(session)) {
        unerror(unmbase_m_set_attribute(session, UNMBASE_MATTR_USER_DATA, VI_NULL));
        unerror(unmbase_m_close(session));
    }

    // ��������� ������ ������������� ��������
    if (is_valid_session(init_session))
        unerror(viClose(session));

Error:
    return status;
}

static unmmko1_command empty_command() {
    unmmko1_command command;
    command.activity = 0;
    command.command_word_1 = 0;
    command.command_word_2 = 0;
    command.data_words_count = 0;
    memset(command.data_words, 0, sizeof(command.data_words));
    return command;
}

message_format_enum detect_bc_rt_format(ViUInt16 command_word) {
    ViUInt16 address, rx_tx, subaddress, word_count;
    unmmko1_unpack_cw(command_word, &address, &rx_tx, &subaddress, &word_count);

    // ���� ����� �� ���������, �� ��� �������� ������� 7-10
    if (31 == address) {
        // ���� �������� 0b00000 ��� 0b11111, �� ��� �������� ������� 9-10
        if (0 == subaddress || 31 == subaddress) {
            // ���� ������� �����, �� ��� �������� ������ 10
            if (0 == rx_tx) {
                // ���� ��� ������� ���������� ��������� � ������������ � �������������, �� ����� ������ 10
                if (17 == word_count || 20 == word_count || 21 == word_count)
                    return f10;
            }
            // ���� ������� ��������, �� ��� �������� ������ 9
            else if (1 == rx_tx) {
                // ���� ��� ������� ���������� ��������� � ������������ � �������������, �� ����� ������ 9
                if (1 == word_count || (3 <= word_count && 8 >= word_count))
                    return f9;
            }
        }
        // �����, ���� ��� �� ������� ����������, �� �������� ������� 7-8
        else {
            // ���� ������� ����� � ������� ���������� ����� ���, �� ��� ������ 7
            if (0 == rx_tx)
                return f7;
        }
    }
    // ����� - ����� ����������� �������, �� ���� �������� ������� 1-6
    else {
        // ���� �������� 0b00000 ��� 0b11111, �� ��� �������� ������� 4-6
        if (0 == subaddress || 31 == subaddress) {
            // ���� ������� �����, �� ��� �������� ������ 6
            if (0 == rx_tx) {
                // ���� ��� ������� ���������� ��������� � ������������ � �������������, �� ����� ������ 6
                if (17 == word_count || 20 == word_count || 21 == word_count)
                    return f6;
            }
            // ���� ������� ��������, �� ��� �������� ������� 4-5
            else if (1 == rx_tx) {
                // ���� ��� ������� ���������� �� 0 �� 8, �� ����� ������ 4
                if (word_count <= 8)
                    return f4;
                // ���� ��� ������� ���������� 16, 18 ��� 19, �� ����� ������ 5
                else if (16 == word_count || 18 == word_count || 19 == word_count)
                    return f5;
            }
        }
        // �����, ���� ��� �� ������� ����������, �� �������� ������� 1-3
        else {
            // ���� ������� ��������, �� ��� ������ 2
            if (1 == rx_tx)
                return f2;
            // ���� ������� ����� � ������� ���������� ����� ���, �� ��� ������ 1
            else
                return f1;
        }
    }

    return f_unknown;
}

//! \brief ������� unmmko1_bc_rt ������ ��������� ������� ��� ��������� ������ ������� ����� ������������ ���� � ��������� �����������.
//! ������� ������������� ��� ������������ ��������� � ����� ��������� ������ - ������� 1, 2, 4, 5, 6, 7, 9, 10.
//! ��������� ���������� ���� ������ ������������ ��������� ������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param command_word ��������� ����� (����� ������� � ������� ������ ������� unmmko1_pack_cw).
//! \param data_words ��������� �� ������ ���� ������ (����� ���� NULL ��� ��������������� ��������).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_bc_rt(unmmko1_bus bus, ViUInt16 command_word, ViUInt16* data_words) {
    unmmko1_command command = empty_command();
    message_format_enum format = detect_bc_rt_format(command_word);

    // ������������� ��������� �������
    if (f_rubbish == format || f_unknown == format || f3 == format || f8 == format)
        return command;

    // ������� (1 � 7) � ����������� ������, ��������� � ��������� �����
    else if (f1 == format || f7 == format) {
        ViUInt16 word_count;
        unmmko1_unpack_cw(command_word, NULL, NULL, NULL, &word_count);

        command.activity = (ViUInt16)(bus | UNMMKO1_MSG_ACT_CWD_1);
        command.command_word_1 = command_word;
        memcpy(command.data_words, data_words, (0 != word_count ? word_count : 32) * sizeof(ViUInt16));
        command.data_words_count = (0 != word_count ? word_count : 32);
        return command;
    }

    // ������� (6 � 10) � ����� ������ ������
    else if (f6 == format || f10 == format) {
        command.activity = (ViUInt16)(bus | UNMMKO1_MSG_ACT_CWD_1);
        command.command_word_1 = command_word;
        memcpy(command.data_words, data_words, sizeof(ViUInt16));
        command.data_words_count = 1;
        return command;
    }

    // ������� ��� ���� ������
    else {
        command.activity = (ViUInt16)(bus | UNMMKO1_MSG_ACT_CWD_1);
        command.command_word_1 = command_word;
        return command;
    }
}

//! \brief ������� unmmko1_rt_rt ������ ��������� ������� ��� ��������� ������ ������� ����� ���������� ������������.
//! ������� ������������� ��� ������������ ��������� � ����� ���������� ������� - ������� 3 � 8.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param command_word_1 ������ ��������� ����� (����� ������� � ������� ������ ������� unmmko1_pack_cw).
//! \param command_word_2 ������ ��������� ����� (����� ������� � ������� ������ ������� unmmko1_pack_cw).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_rt_rt(unmmko1_bus bus, ViUInt16 command_word_1, ViUInt16 command_word_2) {
    // ��������� ��������� �����, ����� ����� ������� ����� 3 � 8 ���������
    ViUInt16 address_1, rx_tx_1, subaddress_1, word_count_1;
    ViUInt16 address_2, rx_tx_2, subaddress_2, word_count_2;
    unmmko1_unpack_cw(command_word_1, &address_1, &rx_tx_1, &subaddress_1, &word_count_1);
    unmmko1_unpack_cw(command_word_2, &address_2, &rx_tx_2, &subaddress_2, &word_count_2);

    // ������ 8 - ��� ���� ������
    if (31 == address_1 && 0 == rx_tx_1 && 0 != subaddress_1 && 31 != subaddress_1 &&
        31 != address_2 && 1 == rx_tx_2 && 0 != subaddress_2 && 31 != subaddress_2 && word_count_1 == word_count_2) {
        unmmko1_command command = empty_command();
        command.activity = (ViUInt16)(bus | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_CWD_2);
        command.command_word_1 = command_word_1;
        command.command_word_2 = command_word_2;
        return command;
    }

    // ������ 3 - � ����������� ������, ��������� � ��������� �����
    else if (31 != address_1 && 0 == rx_tx_1 && 0 != subaddress_1 && 31 != subaddress_1 &&
             31 != address_2 && 1 == rx_tx_2 && 0 != subaddress_2 && 31 != subaddress_2 && word_count_1 == word_count_2) {
        unmmko1_command command = empty_command();
        command.activity = (ViUInt16)(bus | UNMMKO1_MSG_ACT_CWD_1 | UNMMKO1_MSG_ACT_CWD_2);
        command.command_word_1 = command_word_1;
        command.command_word_2 = command_word_2;
        return command;
    }

    // ������������� ��������� �������
    else
        return empty_command();
}

//! \brief ������� unmmko1_f1 ������ ��������� ������� ��������� � ������� 1.
//! ������ 1 - �������� ���� ������ �� ����������� � ���������� ����������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param address ����� ���������� ����������.
//! \param subaddress �������� (1 .. 30).
//! \param word_count ���������� ���� ������.
//! \param data_words ��������� �� ������ ���� ������ (������ ������ ��������� ��� ������� word_count ���� ������).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f1(unmmko1_bus bus, ViUInt16 address, ViUInt16 subaddress, ViUInt16 word_count, ViUInt16* data_words) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(address, 0, subaddress, word_count), data_words);
}

//! \brief ������� unmmko1_f2 ������ ��������� ������� ��������� � ������� 2.
//! ������ 2 - �������� ���� ������ �� ���������� ���������� � �����������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param address ����� ���������� ����������.
//! \param subaddress �������� (1 .. 30).
//! \param word_count ���������� ���� ������.
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f2(unmmko1_bus bus, ViUInt16 address, ViUInt16 subaddress, ViUInt16 word_count) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(address, 1, subaddress, word_count), NULL);
}

//! \brief ������� unmmko1_f3 ������ ��������� ������� ��������� � ������� 3.
//! ������ 3 - �������� ���� ������ �� ���������� ���������� � ���������� ����������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param receive_address ����� ���������� ����������, ������� ��������� ����� ������.
//! \param receive_subaddress �������� ����� ���� ������ (1 .. 30).
//! \param transmit_address ����� ���������� ����������, ������� ������� ����� ������.
//! \param transmit_subaddress �������� �������� ���� ������ (1 .. 30).
//! \param word_count ���������� ���� ������.
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f3(unmmko1_bus bus, ViUInt16 receive_address, ViUInt16 receive_subaddress, ViUInt16 transmit_address, ViUInt16 transmit_subaddress, ViUInt16 word_count) {
    return unmmko1_rt_rt(bus, unmmko1_pack_cw(receive_address, 0, receive_subaddress, word_count), unmmko1_pack_cw(transmit_address, 1, transmit_subaddress, word_count));
}

//! \brief ������� unmmko1_f4 ������ ��������� ������� ��������� � ������� 4.
//! ������ 4 - �������� ������� ���������� ��� ���� ������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param address ����� ���������� ����������.
//! \param subaddress ��������/����� ���������� (SA_MC0 (0) ��� SA_MC31 (31)).
//! \param mode_code ��� ������� ���������� (0 .. 8).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f4(unmmko1_bus bus, ViUInt16 address, ViUInt16 subaddress, ViUInt16 mode_code) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(address, 1, subaddress, mode_code), NULL);
}

//! \brief ������� unmmko1_f5 ������ ��������� ������� ��������� � ������� 5.
//! ������ 5 - �������� ������� ���������� � ���� ����� ������ �� ���������� ����������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param address ����� ���������� ����������.
//! \param subaddress ��������/����� ���������� (SA_MC0 (0) ��� SA_MC31 (31)).
//! \param mode_code ��� ������� ���������� (16, 18, 19).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f5(unmmko1_bus bus, ViUInt16 address, ViUInt16 subaddress, ViUInt16 mode_code) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(address, 1, subaddress, mode_code), NULL);
}

//! \brief ������� unmmko1_f6 ������ ��������� ������� ��������� � ������� 6.
//! ������ 6 - �������� ������� ���������� �� ������ ������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param address ����� ���������� ����������.
//! \param subaddress ��������/����� ���������� (SA_MC0 (0) ��� SA_MC31 (31)).
//! \param mode_code ��� ������� ���������� (17, 20, 21).
//! \param data_word ����� ������.
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f6(unmmko1_bus bus, ViUInt16 address, ViUInt16 subaddress, ViUInt16 mode_code, ViUInt16 data_word) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(address, 0, subaddress, mode_code), &data_word);
}

//! \brief ������� unmmko1_f7 ������ ��������� ������� ��������� � ������� 7.
//! ������ 7 - �������� ���� ������ (� ��������� ���������) �� ����������� ���� � ��������� �����������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param subaddress �������� (1 .. 30).
//! \param word_count ���������� ���� ������.
//! \param data_words ��������� �� ������ ���� ������ (������ ������ ��������� ��� ������� word_count ���� ������).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f7(unmmko1_bus bus, ViUInt16 subaddress, ViUInt16 word_count, ViUInt16* data_words) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(31, 0, subaddress, word_count), data_words);
}

//! \brief ������� unmmko1_f8 ������ ��������� ������� ��������� � ������� 8.
//! ������ 8 - �������� ���� ������ (� ��������� ���������) �� ���������� ���������� � ��������� �����������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param receive_subaddress �������� ����� ���� ������.
//! \param transmit_address ����� ���������� ����������, ������� ������� ����� ������.
//! \param transmit_subaddress �������� �������� ���� ������.
//! \param word_count ���������� ���� ������.
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f8(unmmko1_bus bus, ViUInt16 receive_subaddress, ViUInt16 transmit_address, ViUInt16 transmit_subaddress, ViUInt16 word_count) {
    return unmmko1_rt_rt(bus, unmmko1_pack_cw(31, 0, receive_subaddress, word_count), unmmko1_pack_cw(transmit_address, 1, transmit_subaddress, word_count));
}

//! \brief ������� unmmko1_f9 ������ ��������� ������� ��������� � ������� 9.
//! ������ 9 - �������� ��������� ������� ���������� ��� ���� ������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param subaddress ��������/����� ���������� (SA_MC0 (0) ��� SA_MC31 (31)).
//! \param mode_code ��� ������� ���������� (1, 3, 4, 5, 6, 7, 8).
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f9(unmmko1_bus bus, ViUInt16 subaddress, ViUInt16 mode_code) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(31, 1, subaddress, mode_code), NULL);
}

//! \brief ������� unmmko1_f10 ������ ��������� ������� ��������� � ������� 10.
//! ������ 10 - �������� ��������� ������� ���������� �� ������ ������.
//! \param bus ��������� ���������� ��� �������� ���������� ��������.
//! \param subaddress ��������/����� ���������� (SA_MC0 (0) ��� SA_MC31 (31)).
//! \param mode_code ��� ������� ���������� (17, 20, 21).
//! \param data_word ����� ������.
//! \return ��������� �������.
unmmko1_command _VI_FUNC unmmko1_f10(unmmko1_bus bus, ViUInt16 subaddress, ViUInt16 mode_code, ViUInt16 data_word) {
    return unmmko1_bc_rt(bus, unmmko1_pack_cw(31, 0, subaddress, mode_code), &data_word);
}

static const ViUInt16 terminal_address_mask         = 0xf800;
static const ViUInt16 transmit_receive_field_mask   = 0x0400;
static const ViUInt16 terminal_subaddress_mask      = 0x03e0;
static const ViUInt16 data_count_mask               = 0x001f;
static const ViUInt16 terminal_address_position     = 11;
static const ViUInt16 transmit_receive_position     = 10;
static const ViUInt16 terminal_subaddress_position  = 5;
static const ViUInt16 five_bits_mask                = 0x001f;
static const ViUInt16 error_bits_mask               = 0x07ff;

//! \brief ������� unmmko1_pack_cw ������ 16-������ ��������� ����� �� ������������ ��� �����.
//! \param address ����� ���������� ����������.
//! \param rx_tx ������ �����/�������� (���� - 0, �������� - 1).
//! \param subaddress ��������/����� ����������.
//! \param word_count ���������� ���� ������/��� ������� ����������.
//! \return ��������� �����.
ViUInt16 _VI_FUNC unmmko1_pack_cw(ViUInt16 address, ViUInt16 rx_tx, ViUInt16 subaddress, ViUInt16 word_count) {
    ViUInt16 command_word = (word_count & five_bits_mask);
    command_word |= ((subaddress & five_bits_mask) << terminal_subaddress_position);
    command_word |= ((rx_tx ? 1 : 0) << transmit_receive_position);
    command_word |= ((address & five_bits_mask) << terminal_address_position);
    return command_word;
}

//! \brief ������� unmmko1_unpack_cw ��������� ���� �� 16-������� ���������� �����.
//! ��� ������������� ������ �� ������������ ���������� ��������� �������� NULL ������ ��������� �� ��������.
//! \param[in] command_word ��������� �����.
//! \param[out] address ����� ���������� ����������.
//! \param[out] rx_tx ������ �����/�������� (���� - 0, �������� - 1).
//! \param[out] subaddress ��������/����� ����������.
//! \param[out] word_count ���������� ���� ������/��� ������� ����������.
void _VI_FUNC unmmko1_unpack_cw(ViUInt16 command_word, ViUInt16* address, ViUInt16* rx_tx, ViUInt16* subaddress, ViUInt16* word_count) {
    if (NULL != address)
        *address = ((command_word & terminal_address_mask) >> terminal_address_position);
    if (NULL != rx_tx)
        *rx_tx = (0 != (command_word & transmit_receive_field_mask));
    if (NULL != subaddress)
        *subaddress = ((command_word & terminal_subaddress_mask) >> terminal_subaddress_position);
    if (NULL != word_count)
        *word_count = (command_word & data_count_mask);
}

//! \brief ������� unmmko1_pack_sw ������ 16-������ �������� ����� �� ������������ ��� �����.
//! \param address ����� ���������� ����������.
//! \param error_bits �������� ��������� ����� (����������� �������� �� ������������ unmmko1_error_bits).
//! \return �������� �����.
ViUInt16 _VI_FUNC unmmko1_pack_sw(ViUInt16 address, ViUInt16 error_bits) {
    return (ViUInt16)(((address & five_bits_mask) << 11) | (error_bits & error_bits_mask));
}

//! \brief ������� unmmko1_pack_sw ��������� ���� �� ��������� �����.
//! ��� ������������� ������ �� ������������ ���������� ��������� �������� NULL ������ ��������� �� ��������.
//! \param[in] status_word �������� �����.
//! \param[out] address ����� ���������� ����������.
//! \param[out] error_bits �������� ��������� ����� (����������� �������� �� ������������ unmmko1_error_bits).
void _VI_FUNC unmmko1_unpack_sw(ViUInt16 status_word, ViUInt16* address, ViUInt16* error_bits) {
    if (NULL != address)
        *address = ((status_word & terminal_address_mask) >> terminal_address_position);
    if (NULL != error_bits)
        *error_bits = (status_word & error_bits_mask);
}
