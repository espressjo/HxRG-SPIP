#ifndef INST_TIME_H
#define INST_TIME_H

#include <string>
#include <string.h>
#include <ctime>

std::string ts_now_gmt();
std::string ts_now_local();
double ts_now_jdd();
double ts_now_mjdd();
std::string ts_now_jd();
long gregorian_calendar_to_jd(int y, int m, int d);
double jd_time_now();//sys/time.h implementation
double mjd_time_now();//sys/time.h implementation
std::string jd2str(double mjd);
std::string mjd2str(double mjd);
#endif // INST_TIME_H
