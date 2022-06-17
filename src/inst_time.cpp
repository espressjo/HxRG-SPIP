#include <string>
#include <string.h>
#include <ctime>
#include "inst_time.h"
#include <sys/time.h>


std::string ts_now_gmt()
/*
 * Return a string in with now time YYYY-MM-DDTHH:MM:SS in gmt
 */
{
  time_t rawtime;
  struct tm * ptm;

  time ( &rawtime );

  ptm = gmtime ( &rawtime );
  char tt[20];
  memset(tt,0,20);
  sprintf(tt,"%2.2d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d\n",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
  return std::string(tt);

}

long gregorian_calendar_to_jd(int y, int m, int d)
{
    y+=8000;
    if(m<3) { y--; m+=12; }
    return (y*365) +(y/4) -(y/100) +(y/400) -1200820
          +(m*153+3)/5-92
          +d-1
    ;
}
std::string ts_now_jd()
/*
 * Return a string in with now time in GMT JD
 */
{
  time_t rawtime;
  struct tm * ptm;

  time ( &rawtime );

  ptm = gmtime ( &rawtime );
  int m = ptm->tm_mon+1,y = ptm->tm_year+1900,d =ptm->tm_mday;
  double jd1 = gregorian_calendar_to_jd(y,m,d);
  double  jd2 = (static_cast<double>(ptm->tm_min)/60.0 + static_cast<double>(ptm->tm_hour)  +static_cast<double>(ptm->tm_sec)/3600.0 )/24.0 -0.5;
  char tt[50];
  memset(tt,0,50);
  sprintf(tt,"%f\n",jd1+jd2);
  return std::string(tt);

}
double ts_now_jdd()
/*
 * Return a string in with now time in GMT JD in double
 */
{
  time_t rawtime;
  struct tm * ptm;

  time ( &rawtime );

  ptm = gmtime ( &rawtime );
  int m = ptm->tm_mon+1,y = ptm->tm_year+1900,d =ptm->tm_mday;
  double jd1 = gregorian_calendar_to_jd(y,m,d);
  double  jd2 = (static_cast<double>(ptm->tm_min)/60.0 + static_cast<double>(ptm->tm_hour)  +static_cast<double>(ptm->tm_sec)/3600.0 )/24.0 -0.5;

  return jd1+jd2;

}
double ts_now_mjdd()
/*
 * A modified version of the Julian date denoted MJD obtained
 * by subtracting 2,400,000.5 days from the Julian date JD.
 * Return a double.
 */
{
  time_t rawtime;
  struct tm * ptm;

  time ( &rawtime );

  ptm = gmtime ( &rawtime );
  int m = ptm->tm_mon+1,y = ptm->tm_year+1900,d =ptm->tm_mday;
  double jd1 = gregorian_calendar_to_jd(y,m,d);
  double  jd2 = (static_cast<double>(ptm->tm_min)/60.0 + static_cast<double>(ptm->tm_hour)  +static_cast<double>(ptm->tm_sec)/3600.0 )/24.0 -0.5;

  return (jd1+jd2)-2400000.5;

}
std::string ts_now_local()
/*
 * Return a string in with now time YYYY-MM-DDTHH:MM:SS in gmt
 */
{
    time_t t = time(nullptr);
    struct tm * ptm = localtime(&t);
  char tt[20];
  memset(tt,0,20);
  sprintf(tt,"%2.2d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d\n",ptm->tm_year+1900,ptm->tm_mon+1,ptm->tm_mday,ptm->tm_hour,ptm->tm_min,ptm->tm_sec);
  return std::string(tt);

}
double jd_time_now()
/*!
  * Based on sys/time.h
  * return jd in double
  */
{
    struct timeval tv;
    if (gettimeofday(&tv,nullptr)!=0)
    {
        return -1;
    }
    double jd;
    jd = 2440587.5 + ((static_cast<double>(tv.tv_sec) +(static_cast<double>(tv.tv_usec)/1000000.0))/86400.0);
    return jd;
}
double mjd_time_now()
/*!
  * Based on sys/time.h
  * A modified version of the Julian date denoted MJD obtained
  * by subtracting 2,400,000.5 days from the Julian date JD.
  * Return a double
  */
{
    struct timeval tv;
    if (gettimeofday(&tv,nullptr)!=0)
    {
        return -1;
    }
    double mjd;
    mjd = 2440587.5 + ((static_cast<double>(tv.tv_sec) +(static_cast<double>(tv.tv_usec)/1000000.0))/86400.0);
    return mjd-2400000.5;
}
std::string mjd2str(double mjd)
/*!
  * Based on sys/time.h
  * Convert a MJD double in std::string in
  * the format of yyy-mm-ddThh:mm:ss
  */
{
    mjd+=(2400000.5-2440587.5);
    struct timeval tv;
    mjd*=86400.0;
    tv.tv_sec = static_cast<long>(mjd);
    double usec = (mjd-static_cast<long>(mjd))*1000000.0;
    tv.tv_usec=static_cast<long>(usec);

    struct tm *tm;
    time_t tt = tv.tv_sec;
    tm = gmtime(&tt);
    char txttime[20];
    memset(txttime,0,20);
    sprintf(txttime,"%2.2d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);

    return std::string(txttime);
}
std::string jd2str(double mjd)
/*!
  * Based on sys/time.h
  * Convert a JD double in std::string in
  * the format of yyy-mm-ddThh:mm:ss
  */
{
    mjd-=(2440587.5);
    struct timeval tv;
    mjd*=86400.0;
    tv.tv_sec = static_cast<long>(mjd);
    double usec = (mjd-static_cast<long>(mjd))*1000000.0;
    tv.tv_usec=static_cast<long>(usec);

    struct tm *tm;
    time_t tt = tv.tv_sec;
    tm = gmtime(&tt);
    char txttime[20];
    memset(txttime,0,20);
    sprintf(txttime,"%2.2d-%2.2d-%2.2dT%2.2d:%2.2d:%2.2d",tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,tm->tm_hour,tm->tm_min,tm->tm_sec);

    return std::string(txttime);
}
