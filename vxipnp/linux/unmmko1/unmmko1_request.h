#ifndef UNMMKO1_REQUEST_H
#define UNMMKO1_REQUEST_H

#include "unmmko1.h"

//! ����� �������
#define REQUEST_ADDRESS 4
//! ������ �������
#define REQUEST_SIZE 30
//! ������ ����� ������ � �������
#define REQUEST_DATA_SIZE 27
//! �������� ����� ���������� � �������
#define REQUEST_INCREMENT 1

//! ��������� ������
typedef struct _unmmko1_request {
    ViUInt16 new_data;
    ViUInt16 data[REQUEST_DATA_SIZE];
    ViUInt16 command;
    ViUInt16 counter;
} unmmko1_request_data;

//! ����� ������
#define RESPONSE_ADDRESS 4
//! ������ ������ �� ������
#define RESPONSE_SIZE REQUEST_SIZE

//! ��������� �����
typedef ViUInt16 unmmko1_response_data[RESPONSE_SIZE];

//! ���������� ������� request � ��������� ������ response
ViStatus execute_request(ViSession session, unmmko1_request_data* request, unmmko1_response_data* response);

//! ��������� �������
void clear_request(unmmko1_request_data* request);

//! ��������� ������
void clear_response(unmmko1_response_data* response);

#endif // UNMMKO1_REQUEST_H
