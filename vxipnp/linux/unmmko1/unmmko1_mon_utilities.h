#ifndef UNMMKO1_MON_UTILITIES_H
#define UNMMKO1_MON_UTILITIES_H

#include "unmmko1_internal.h"

//! ������ ���� ��������.
//! \param[in] words ������� � ������ ������� � ��������.
//! \param[in] accumulator ����������� ����, ������� ������������ ��� ���������� ���� � �������� �������.
//! \param[out] messages �������, � ������� ���������� ����������� ���������.
void unmmko1_parse_words(un_queue* words, accumulator_t* accumulator, un_queue* messages);

//! ���������� ��������� �� �������� ��������.
//! \param[in] messages ������� ���������.
//! \param[in] filter_rules ������� ���������� ���������.
//! \param[out] filtered_messages ��������������� ������� ���������.
void unmmko1_filter_messages(un_queue* messages, filter_rules_t* filter_rules, un_queue* filtered_messages);

#endif // UNMMKO1_MON_UTILITIES_H
