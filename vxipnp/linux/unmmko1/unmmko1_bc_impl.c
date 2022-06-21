#include "unmmko1.h"
#include "internal/unmmko1_internal.h"
#include <stdlib.h>

//! \brief ������� unmmko1_bc_configure ������������ ��������� �������� � ������ ����������� ����.
//! ����� ������� ����� ������������ ����� �������� �����������.
//! ���� ������� ��� ������� � �����-���� ������ ������, �� ����� ����� ����������� ��������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \param bc_options ��������� ����������� ���� - ����������� ������ UNMMKO1_BC_* �� ������������ unmmko1_bc_options.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_configure(ViSession session, int bc_options) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode && device_data->bus_controller.is_started)
        unerror(unmmko1_bc_stop(session));
    else if (UNMMKO1_MODE_REMOTE_TERMINAL == device_data->current_mode && device_data->remote_terminal.is_started)
        unerror(unmmko1_rt_stop(session));
    else if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode && device_data->bus_monitor.is_started)
        unerror(unmmko1_mon_stop(session));

    unerror(unmmko1_bc_reset(session));
    unerror(set_current_mode(session, UNMMKO1_MODE_BUS_CONTROLLER));
    unerror(set_connection_type(session, (bc_options & UNMMKO1_BC_DIRECT) ? UNMMKO1_DIRECT : UNMMKO1_TRANSFORM));
    device_data->bus_controller.options = bc_options;

Error:
    return status;
}

//! \brief ������� unmmko1_bc_reset ������������ ����� �������� ����������� ����: �������� ��������� ��������� �� ����������, ���������� ���������� ������������ � 1.
//! ���� ������� ��� ������� � �����-���� ������ ������, �� ����� ����� ����������� ��������� ��������.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_reset(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode && device_data->bus_controller.is_started)
        unerror(unmmko1_bc_stop(session));
    else if (UNMMKO1_MODE_REMOTE_TERMINAL == device_data->current_mode && device_data->remote_terminal.is_started)
        unerror(unmmko1_rt_stop(session));
    else if (UNMMKO1_MODE_BUS_MONITOR == device_data->current_mode && device_data->bus_monitor.is_started)
        unerror(unmmko1_mon_stop(session));

    reset_bus_controller_settings(&device_data->bus_controller);

Error:
    return status;
}

//! \brief ������� unmmko1_bc_schedule_command ��������� ��������� ������� � ����� ����������.
//! ���������� ��������� ��������� � ����������, ���������� �� ���������� ���������� ����������, �� ������ ��������� UNMMKO1_BC_SCHEDULE_SIZE_LIMIT.
//! \param session ����� ������ ����� � ���������.
//! \param command ��������� ������� ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_schedule_command(ViSession session, unmmko1_command command) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (device_data->bus_controller.is_started)
        return UNMMKO1_ERROR_ILLEGAL_OPERATION_WHILE_RUNNING;

    if ((un_queue_size(device_data->bus_controller.schedule) + 1) * MAX((uint16_t)1, device_data->bus_controller.repeat_count) > UNMMKO1_BC_SCHEDULE_SIZE_LIMIT)
        return UNMMKO1_ERROR_EXCEEDED_SCHEDULE_SIZE_LIMIT;

    un_queue_push_back(device_data->bus_controller.schedule, &command);

    return VI_SUCCESS;
}

//! \brief ������� unmmko1_bc_set_schedule_repeat_count ������������� ���������� ���������� ����������.
//! ���������� ��������� ��������� � ����������, ���������� �� ���������� ���������� ����������, �� ������ ��������� UNMMKO1_BC_SCHEDULE_SIZE_LIMIT.
//! \param session ����� ������ ����� � ���������.
//! \param repeat_count ���������� ���������� ���������� (�������� 0 �������� ����������� ���������� ����������).
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_set_schedule_repeat_count(ViSession session, ViUInt16 repeat_count) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (device_data->bus_controller.is_started)
        return UNMMKO1_ERROR_ILLEGAL_OPERATION_WHILE_RUNNING;

    if (un_queue_size(device_data->bus_controller.schedule) * MAX(1, repeat_count) > UNMMKO1_BC_SCHEDULE_SIZE_LIMIT)
        return UNMMKO1_ERROR_EXCEEDED_SCHEDULE_SIZE_LIMIT;

    device_data->bus_controller.repeat_count = repeat_count;

    return VI_SUCCESS;
}

//! \brief ������� unmmko1_bc_start ��������� ������� � ������ ����������� ����.
//! � ����������� �� ����, ���� �� ����� ����������� ����������, ����� �������������� ������ � ������ ���������� ��� � ������ �������� ��������� ��������� ���������.
//! ��������� ����������� ���� �������������� ��������������� �������� unmmko1_bc_configure.
//! ���������� ������������� ��������� unmmko1_bc_schedule_command � unmmko1_bc_set_schedule_repeat_count.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_start(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_UNDEFINED == device_data->current_mode)
        unerror(unmmko1_bc_configure(session, UNMMKO1_BC_DEFAULT));

    if (UNMMKO1_MODE_BUS_CONTROLLER != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (device_data->bus_controller.is_started)
        return UNMMKO1_WARNING_ALREADY_STARTED;

    // ���� ���������� ������, �� ��������� ��� � ������
    if (0 != un_queue_size(device_data->bus_controller.schedule)) {
        // �������� ���������� ���������� ����������
        if (0 != device_data->bus_controller.repeat_count) {
            ViUInt16 repeat_index = 0;
            for (repeat_index = 0; repeat_index < device_data->bus_controller.repeat_count; ++repeat_index) {
                ViUInt16 command_index = 0;
                unmmko1_command command;
                const ViUInt16 schedule_size = (ViUInt16)un_queue_size(device_data->bus_controller.schedule);
                for (command_index = 0; command_index < schedule_size; ++command_index) {
                    ViUInt16 current_command_index = (repeat_index * schedule_size) + command_index;
                    ViUInt16 next_command_index = current_command_index + 1;
                    ViBoolean is_last = (command_index + 1 == schedule_size) && (repeat_index + 1 == device_data->bus_controller.repeat_count);

                    un_queue_get(device_data->bus_controller.schedule, command_index, &command);
                    unerror(set_bc_command(session, current_command_index, command, is_last, next_command_index));
                }
            }
        }
        // ���������� ����������� ����������
        else {
            ViUInt16 command_index = 0;
            unmmko1_command command;
            const ViUInt16 schedule_size = (ViUInt16)un_queue_size(device_data->bus_controller.schedule);
            for (command_index = 0; command_index < schedule_size; ++command_index) {
                ViUInt16 current_command_index = command_index;
                ViUInt16 next_command_index = (command_index + 1 == schedule_size) ? 0 : current_command_index + 1;

                un_queue_get(device_data->bus_controller.schedule, command_index, &command);
                unerror(set_bc_command(session, command_index, command, VI_FALSE, next_command_index));
            }
        }

        unerror(start_bc_mon(session));
        unerror(start_bc(session, 0));
        device_data->bus_controller.is_started = VI_TRUE;
    }
    // � ��������� ������ ��������� � ����� �������� ��������� ������
    else {
        unerror(start_bc_mon(session));
        device_data->bus_controller.is_started = VI_TRUE;
    }

Error:
    return status;
}

//! \brief ������� unmmko1_bc_status ���������� ��������� ������� �������� � ������ ����������� ����.
//! ������� ���������� VI_TRUE ����� �������� ������� ����������� ���� � ������� unmmko1_bc_start.
//! ������ ��������� ������� �� ����� �������������� ��� ������� ���������� ���������� ����������.
//! \param session ����� ������ ����� � ���������.
//! \param status ��������� ������� ����������� ���� (VI_TRUE - ���������� ���� �������, VI_FALSE - ����������).
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_status(ViSession session, ViBoolean* status) {
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (is_not_valid_pointer(status))
        return VI_ERROR_PARAMETER2;

    *status = device_data->bus_controller.is_started;
    return VI_SUCCESS;
}

//! \brief ������� unmmko1_bc_stop ������������� �������, ���������� ����� � ������ ����������� ����.
//! \param session ����� ������ ����� � ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_stop(ViSession session) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_BUS_CONTROLLER != device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (!device_data->bus_controller.is_started)
        return UNMMKO1_WARNING_ALREADY_STOPPED;

    unerror(stop_bc(session));
    device_data->bus_controller.is_started = VI_FALSE;

Error:
    return status;
}

//! \brief ������� unmmko1_bc_transmit_command ���������� ������� ��������� ������� ���������.
//! \remark ������� �������� ���������� �������� ���������� � ���������� ������, ���� ����� ���� ��������� ����������.
//! \param session ����� ������ ����� � ���������.
//! \param command ��������� ������� ���������.
//! \return ��� ������ ��� ��� ��������� ���������� �������.
ViStatus _VI_FUNC unmmko1_bc_transmit_command(ViSession session, unmmko1_command command) {
    ViStatus status = VI_SUCCESS;
    unmmko1_device_data* device_data = get_device_data_or_error(session);

    if (UNMMKO1_MODE_UNDEFINED == device_data->current_mode)
        return UNMMKO1_ERROR_WAS_CONFIGURED_IN_A_DIFFERENT_MODE;

    if (UNMMKO1_MODE_BUS_CONTROLLER == device_data->current_mode) {
        if (!device_data->bus_controller.is_started)
            return UNMMKO1_ERROR_ILLEGAL_OPERATION_WHILE_STOPPED;

        if (0 != un_queue_size(device_data->bus_controller.schedule))
            return UNMMKO1_ERROR_ILLEGAL_OPERATION_IN_SCHEDULE_MODE;
    }

    unerror(transmit_bc_command(session, command));

Error:
    return status;
}
