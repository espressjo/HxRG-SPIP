#ifndef PHEADER_H
#define PHEADER_H

#include "macie.h"
#include <string>
#include <vector>
#include <string.h>
class pHeader
{
public:
    pHeader();
    pHeader(MACIE_FitsHdr *pHeaders,int length_header);
    ~pHeader();

    void add_entry(std::string key,int value, std::string comment);
    void add_entry(std::string key,bool value, std::string comment);
    void add_entry(std::string key,double value, std::string comment);
    void add_entry(std::string key,std::string value, std::string comment);
    MACIE_FitsHdr* operator[](std::string key);
    MACIE_FitsHdr* get_header();
    uint16_t get_header_length();
    void edit_entry(std::string entry,std::string value);
    void edit_entry(std::string entry,bool value);
    void edit_entry(std::string entry,float value);
    void edit_entry(std::string entry,double value);
    void edit_entry(std::string entry,int value);

private:
    MACIE_FitsHdr *h;
    std::vector<MACIE_FitsHdr> head_v;
};

#endif // PHEADER_H
