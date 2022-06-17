#include "uics_base64.h"




static unsigned char to_uchar(char ch)
{
    return ch;
}

void islb64Encode(const char *in, size_t inlen,char *out, size_t outlen)
/**
 * Base64 encode array.
 *
 * It base64 encode \em in array of size \em inlen into \em out array of size
 * \em outlen. If \em outlen is less than islb64_LENGTH(inlen), write as
 * many bytes as possible. If \em outlen is larger than
 * islb64_LENGTH(inlen), also zero terminate the output buffer.
 *
 * @param in array to encode
 * @param inlen size of input array
 * @param out array where base64 encoded data will be stored
 * @param outlen size of output array
 */
{
    const char b64[65] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    while (inlen && outlen)
    {
        *out++ = b64[to_uchar (in[0]) >> 2];
        if (!--outlen)
        {
            break;
        }
        *out++ = b64[((to_uchar (in[0]) << 4)
                      + (--inlen ? to_uchar (in[1]) >> 4 : 0)) & 0x3f];
        if (!--outlen)
        {
            break;
        }
        *out++ =
            (inlen
             ? b64[((to_uchar (in[1]) << 2)
                    + (--inlen ? to_uchar (in[2]) >> 6 : 0)) & 0x3f] : '=');
        if (!--outlen)
        {
            break;
        }
        *out++ = inlen ? b64[to_uchar (in[2]) & 0x3f] : '=';
        if (!--outlen)
        {
            break;
        }
        if (inlen)
        {
            inlen--;
        }
        in += 3;
    }

    if (outlen)
    {
        *out = '\0';
    }
}

size_t islb64EncodeAlloc(const char *in, size_t inlen, char **out)
/**
 * Allocate a buffer and store zero terminated base64 encoded data.
 *
 * It allocates a buffer and stores zero terminated base64 encoded data from
 * array \em in of size \em inlen, returning islb64_LENGTH(inlen), i.e.,
 * the length of the encoded data, excluding the terminating zero. On return,
 * the \em out variable will hold a pointer to newly allocated memory that
 * must be deallocated by the caller. If output string length would overflow,
 * 0 is returned and \em out is set to NULL. If memory allocation fail, \em
 * out is set to NULL, and the return value indicate length of the requested
 * memory block, i.e., islb64_LENGTH(inlen) + 1.
 *
 * \param in array to encode
 * \param inlen size of input array
 * \param out pointer to the allocated memory containing encoded data
 *
 * @return size of the allocated memory or 0 if an error occured
 */
{
    size_t outlen = 1 + islb64LENGTH (inlen);

    /* Check for overflow in outlen computation.
     *
     * If there is no overflow, outlen >= inlen.
     *
     * If the operation (inlen + 2) overflows then it yields at most +1, so
     * outlen is 0.
     *
     * If the multiplication overflows, we lose at least half of the
     * correct value, so the result is < ((inlen + 2) / 3) * 2, which is
     * less than (inlen + 2) * 0.66667, which is less than inlen as soon as
     * (inlen > 4).
     */
    if (inlen > outlen)
    {
        //errAdd(error, "islb64", islb64ERR_BASE64_OVERFLOW, __FILE_LINE__, NULL);
        //*out = NULL;
        return 0;
    }

    *out = static_cast<char*>(malloc(outlen));
    if (*out == nullptr)
    {
        return 0;
    }
    islb64Encode(in, inlen, *out, outlen);

    return outlen - 1;
}



int islb64Decode(const char *in, size_t inlen,char *out, size_t * outlen)
/**
 * Decode a base64 encoded input array,
 *
 * It decodes a base64 encoded input array \em in of length \em inlen to
 * output array \em out that can hold \em outlen bytes. Return true if
 * decoding was successful, i.e. if the input was valid base64 data, false
 * otherwise. If \em outlen is too small, as many bytes as possible will be
 * written to \em out. On return, \em outlen holds the length of decoded bytes
 * in \em out. Note that as soon as any non-alphabet characters are
 * encountered, decoding is stopped and false is returned.
 *
 * @param in array to decode
 * @param inlen size of input array
 * @param out array where base64 encoded data will be stored
 * @param outlen size of output array
 *
 * @return FAILURE in case of error, SUCCESS otherwise
 */
{
    size_t outleft = *outlen;

    while (inlen >= 2)
    {
        if ((islb64IsBase64(in[0]) == false)||
            (islb64IsBase64(in[1]) == false))
        {
            break;
        }
        if (outleft)
        {
            *out++ = ((b64[to_uchar (in[0])] << 2)
                      | (b64[to_uchar (in[1])] >> 4));
            outleft--;
        }

        if (inlen == 2)
        {
            break;
        }

        if (in[2] == '=')
        {
            if (inlen != 4)
            {
                break;
            }

            if (in[3] != '=')
            {
                break;
            }
        }
        else
        {
            if (islb64IsBase64(in[2]) == false)
            {
                break;
            }
            if (outleft)
            {
                *out++ = (((b64[to_uchar (in[1])] << 4) & 0xf0)
                          | (b64[to_uchar (in[2])] >> 2));
                outleft--;
            }

            if (inlen == 3)
            {
                break;
            }

            if (in[3] == '=')
            {
                if (inlen != 4)
                {
                    break;
                }
            }
            else
            {
                if (islb64IsBase64(in[3]) == false)
                {
                    break;
                }

                if (outleft)
                {
                    *out++ = (((b64[to_uchar (in[2])] << 6) & 0xc0)
                              | b64[to_uchar (in[3])]);
                    outleft--;
                }
            }
        }

        in += 4;
        inlen -= 4;
    }

    *outlen -= outleft;

    if (inlen != 0)
    {

        return -1;
    }

    return 0;
}


int islb64DecodeAlloc(const char *in, size_t inlen,char **out, size_t * outlen)
/**
 * Allocate a buffer, and decode the base64 encoded data.
 *
 * It allocates an output buffer in \em out, and decodes the base64 encoded
 * data stored in IN of size \em inlen to the \em out buffer. On return, the
 * size of the decoded data is stored in \em outlen. \em outlen may be NULL,
 * if the caller is not interested in the decoded length. \em out may be NULL
 * to indicate an out of memory error, in which case \em outlen contain the
 * size of the memory block needed. The function return true on successful
 * decoding and memory allocation errors. (Use the \em out and \em outlen
 * parameters to differentiate between successful decoding and memory error.)
 * The function return false if the input was invalid, in which case \em out
 * is NULL and \em outlen is undefined.
 *
 * \param in array to decode
 * \param inlen size of input array
 * \param out pointer to the allocated memory containing decoded data
 * \param outlen size of the allocated memory
 *
 * @return FAILURE in case of error, SUCCESS otherwise
 */
{
    /* This may allocate a few bytes too much, depending on input,
       but it's not worth the extra CPU time to compute the exact amount.
       The exact amount is 3 * inlen / 4, minus 1 if the input ends
       with "=" and minus another 1 if the input ends with "==".
       Dividing before multiplying avoids the possibility of overflow.  */
    size_t needlen = 3 * (inlen / 4) + 2;

    *out = static_cast<char*>(malloc(needlen));
    if (!*out)
    {

        return -1;
    }

    if (islb64Decode(in, inlen, *out, &needlen) ==-1)
    {
        free (*out);
        *out = nullptr;
        return -1;
    }

    if (outlen)
    {
        *outlen = needlen;
    }
    return 0;
}


/**
 * Check whether the character is a base64 encoded character or not.
 *
 * @param ch character to test
 *
 * @return ccsTRUE if it is a base64 encoded character, ccsFALSE
 * otherwise
 */
bool islb64IsBase64(char ch)
{
    if (0 <= b64[to_uchar(ch)])
    {
        return true;
    }
    else
    {
        return false;
    }
}

