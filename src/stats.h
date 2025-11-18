#pragma once
#include <string>

void stats_inc_connection();
void stats_dec_connection();
void stats_inc_command();
void stats_inc_error();

std::string stats_render();
