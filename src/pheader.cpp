#include "pheader.h"

pHeader::~pHeader()
/*
 * Destructore.
 */
{

    delete [] h;

}
uint16_t pHeader::get_header_length()
/*
 * Return the number of entries in pHeader.
 * This is use with MACIE's function to write
 * FITS file.
 */
{
    return static_cast<uint16_t>(head_v.size());
}
MACIE_FitsHdr* pHeader::get_header()
/*
 * Return the old style header (MACIE structure).
 * This is use with MACIE's function to write
 * FITS file.
 */
{
    h = new MACIE_FitsHdr[head_v.size()];
    for (unsigned long i=0;i<head_v.size();i++) {
        h[i] = head_v[i];
    }
    return h;
}
pHeader::pHeader(MACIE_FitsHdr *pHeaders,int length_header)
/*
 * We can initialize a pHeader with the MACIE header
 * structure. The advantage of using pHeader instead of
 * MACIE's structure is the ease to add and edit new
 * entries.
 */
{
    for (int i=0;i<length_header;i++) {
        head_v.push_back(pHeaders[i]);
    }

}
pHeader::pHeader()
/*
 * The advantage of using pHeader instead of
 * MACIE's structure is the ease to add and edit new
 * entries.
 */
{

}
void pHeader::add_entry(std::string key,int value, std::string comment)
/*
 * Add an entry to the header. Be carefull not to call this function
 * in a loop because it will create several entry with the same name.
 * Instead, use edit_entry() inside a loop.
 */
{
    MACIE_FitsHdr entry;
    if (key.length()>=9)
    {
        strncpy(entry.key, key.c_str(), 9);
    }
    else {
        memset(entry.key,0,9);
        strncpy(entry.key, key.c_str(), key.length());
    }
    entry.iVal =  value;
    entry.valType = HDR_INT;
    memset(entry.comment,0,72);
    printf("size of comment: %ld\n",sizeof(comment.c_str()));
    if (comment.length()<72)
    {
    strncpy(entry.comment, static_cast<const char*>(comment.c_str()), comment.length());
    }
    else {
        strncpy(entry.comment, static_cast<const char*>(comment.c_str()), 72);
    }

    head_v.push_back(entry);
}
void pHeader::add_entry(std::string key,bool value, std::string comment)
/*
 * Add an entry to the header. Be carefull not to call this function
 * in a loop because it will create several entry with the same name.
 * Instead, use edit_entry() inside a loop.
 */
{
    MACIE_FitsHdr entry;
    if (key.length()>=9)
    {
        strncpy(entry.key, key.c_str(), 9);
    }
    else {
        memset(entry.key,0,9);
        strncpy(entry.key, key.c_str(), key.length());
    }
    entry.iVal =  (value) ? 1: 0;
    entry.valType = HDR_INT;

    memset(entry.comment,0,72);
    printf("size of comment: %ld\n",sizeof(comment.c_str()));
    if (comment.length()<72)
    {
    strncpy(entry.comment, static_cast<const char*>(comment.c_str()), comment.length());
    }
    else {
        strncpy(entry.comment, static_cast<const char*>(comment.c_str()), 72);
    }

    head_v.push_back(entry);
}
void pHeader::add_entry(std::string key,double value, std::string comment)
/*
 * Add an entry to the header. Be carefull not to call this function
 * in a loop because it will create several entry with the same name.
 * Instead, use edit_entry() inside a loop.
 */
{
    MACIE_FitsHdr entry;
    if (key.length()>=9)
    {
        strncpy(entry.key, key.c_str(), 9);
    }
    else {
        memset(entry.key,0,9);
        strncpy(entry.key, key.c_str(), key.length());
    }
    entry.fVal =  static_cast<float>(value);
    entry.valType = HDR_FLOAT;
    memset(entry.comment,0,72);
    if (comment.length()<72)
    {
    strncpy(entry.comment, static_cast<const char*>(comment.c_str()), comment.length());
    }
    else {
        strncpy(entry.comment, static_cast<const char*>(comment.c_str()), 72);
    }
    head_v.push_back(entry);
}
void pHeader::add_entry(std::string key,std::string value, std::string comment)
/*
 * Add an entry to the header. Be carefull not to call this function
 * in a loop because it will create several entry with the same name.
 * Instead, use edit_entry() inside a loop.
 */
{
    MACIE_FitsHdr entry;
    if (key.length()>=9)
    {
        strncpy(entry.key, key.c_str(), 9);
    }
    else {
        memset(entry.key,0,9);
        strncpy(entry.key, key.c_str(), key.length());
    }
    memset(entry.sVal,0,72);
    if (value.length()<72)
    {
        strncpy(entry.sVal, static_cast<const char*>(value.c_str()), value.length());
    }
    else {
        strncpy(entry.sVal, static_cast<const char*>(value.c_str()), 72);
    }

    entry.valType = HDR_STR;
    memset(entry.comment,0,72);
    if (comment.length()<72)
    {
    strncpy(entry.comment, static_cast<const char*>(comment.c_str()), comment.length());
    }
    else {
        strncpy(entry.comment, static_cast<const char*>(comment.c_str()), 72);
    }
    head_v.push_back(entry);
}
MACIE_FitsHdr* pHeader::operator[](std::string key)
/*
 * This function returns a MACIE's header structure.
 * It could be used to edit a single header entry.
 * e.g., header["RAMP"]->iVal = 21. However, it is
 * better to use the utility function edit_entry()
 * to do so.
 */
{
    for (unsigned long i=0;i<head_v.size();i++) {
        if (key.compare(head_v[i].key)==0)
        {
            return &head_v[i];
        }
    }
    return nullptr;
}

void pHeader::edit_entry(std::string entry,std::string value)
/*
 * Edit an existing entry. If the entry does not exist
 * the function will still return without an error.
 */
{
    for (unsigned long i=0;i<head_v.size();i++) {
        if (entry.compare(head_v[i].key)==0)
        {
            strncpy(head_v[i].sVal,value.c_str(),sizeof (head_v[i].sVal));
            return ;
        }
    }
    return ;
}
void pHeader::edit_entry(std::string entry,int value)
/*
 * Edit an existing entry. If the entry does not exist
 * the function will still return without an error.
 */
{
    for (unsigned long i=0;i<head_v.size();i++) {
        if (entry.compare(head_v[i].key)==0)
        {
            head_v[i].iVal = value;
            return ;
        }
    }
    return ;
}
void pHeader::edit_entry(std::string entry,bool value)
/*
 * Edit an existing entry. If the entry does not exist
 * the function will still return without an error.
 */
{
    for (unsigned long i=0;i<head_v.size();i++) {
        if (entry.compare(head_v[i].key)==0)
        {
            head_v[i].iVal = (value) ? 1: 0;
            return ;
        }
    }
    return ;
}
void pHeader::edit_entry(std::string entry,float value)
/*
 * Edit an existing entry. If the entry does not exist
 * the function will still return without an error.
 */
{
    for (unsigned long i=0;i<head_v.size();i++) {
        if (entry.compare(head_v[i].key)==0)
        {
            head_v[i].fVal = value;
            return ;
        }
    }
    return ;
}
void pHeader::edit_entry(std::string entry,double value)
/*
 * Edit an existing entry. If the entry does not exist
 * the function will still return without an error.
 */
{
    for (unsigned long i=0;i<head_v.size();i++) {
        if (entry.compare(head_v[i].key)==0)
        {
            head_v[i].fVal = static_cast<float>(value);
            return ;
        }
    }
    return ;
}
