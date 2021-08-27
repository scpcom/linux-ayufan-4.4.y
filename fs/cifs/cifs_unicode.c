#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/slab.h>
#include "cifs_unicode.h"
#include "cifs_uniupr.h"
#include "cifspdu.h"
#include "cifsglob.h"
#include "cifs_debug.h"

int
cifs_ucs2_bytes(const __le16 *from, int maxbytes,
		const struct nls_table *codepage)
{
	int i;
	int charlen, outlen = 0;
	int maxwords = maxbytes / 2;
	char tmp[NLS_MAX_CHARSET_SIZE];
	__u16 ftmp;

	for (i = 0; i < maxwords; i++) {
		ftmp = get_unaligned_le16(&from[i]);
		if (ftmp == 0)
			break;

		charlen = codepage->uni2char(ftmp, tmp, NLS_MAX_CHARSET_SIZE);
		if (charlen > 0)
			outlen += charlen;
		else
			outlen++;
	}

	return outlen;
}

static int
cifs_mapchar(char *target, const __u16 src_char, const struct nls_table *cp,
	     bool mapchar)
{
	int len = 1;

	if (!mapchar)
		goto cp_convert;

	switch (src_char) {
	case UNI_COLON:
		*target = ':';
		break;
	case UNI_ASTERISK:
		*target = '*';
		break;
	case UNI_QUESTION:
		*target = '?';
		break;
	case UNI_PIPE:
		*target = '|';
		break;
	case UNI_GRTRTHAN:
		*target = '>';
		break;
	case UNI_LESSTHAN:
		*target = '<';
		break;
#ifdef MY_ABC_HERE
	case UNI_DQUOT:
		*target = '"';
		break;
	case UNI_DIVSLASH:
		*target = '/';
		break;
	case UNI_CRGRET:
		*target = '\r';
		break;
#endif
	default:
		goto cp_convert;
	}

out:
	return len;

cp_convert:
	len = cp->uni2char(src_char, target, NLS_MAX_CHARSET_SIZE);
	if (len <= 0) {
		*target = '?';
		len = 1;
	}
	goto out;
}

int
cifs_from_ucs2(char *to, const __le16 *from, int tolen, int fromlen,
		 const struct nls_table *codepage, bool mapchar)
{
	int i, charlen, safelen;
	int outlen = 0;
	int nullsize = nls_nullsize(codepage);
	int fromwords = fromlen / 2;
	char tmp[NLS_MAX_CHARSET_SIZE];
	__u16 ftmp;

	safelen = tolen - (NLS_MAX_CHARSET_SIZE + nullsize);

	for (i = 0; i < fromwords; i++) {
		ftmp = get_unaligned_le16(&from[i]);
		if (ftmp == 0)
			break;

		if (outlen >= safelen) {
			charlen = cifs_mapchar(tmp, ftmp, codepage, mapchar);
			if ((outlen + charlen) > (tolen - nullsize))
				break;
		}

		charlen = cifs_mapchar(&to[outlen], ftmp, codepage, mapchar);
		outlen += charlen;
	}

	for (i = 0; i < nullsize; i++)
		to[outlen++] = 0;

	return outlen;
}

#ifdef MY_ABC_HERE
int
cifs_strtoUCS_NoSpecialChar(__le16 *to, const char *from, int len,
	      const struct nls_table *codepage)
{
	int charlen;
	int i;
	wchar_t wchar_to;  

	for (i = 0; len && *from; i++, from += charlen, len -= charlen) {
		charlen = codepage->char2uni(from, len, &wchar_to);
		if (charlen < 1) {
			cERROR(1, "strtoUCS: char2uni of 0x%x returned %d",
				*from, charlen);
			 
			wchar_to = 0x003f;
			charlen = 1;
		}
		put_unaligned_le16(wchar_to, &to[i]);
	}

	put_unaligned_le16(0, &to[i]);
	return i;
}
#endif

int
cifs_strtoUCS(__le16 *to, const char *from, int len,
	      const struct nls_table *codepage)
{
	int charlen;
	int i;
	wchar_t wchar_to;  

	for (i = 0; len && *from; i++, from += charlen, len -= charlen) {
#ifdef MY_ABC_HERE
		if (0x0d == *from) {     
			to[i] = cpu_to_le16(0xf00d);
			charlen = 1;
		} else if (0x2a == *from) {      
			to[i] = cpu_to_le16(0xf02a);
			charlen = 1;
		} else if (0x2f == *from) {      
			to[i] = cpu_to_le16(0xf02f);
			charlen = 1;
		} else if (0x3c == *from) {      
			to[i] = cpu_to_le16(0xf03c);
			charlen = 1;
		} else if (0x3e == *from) {      
			to[i] = cpu_to_le16(0xf03e);
			charlen = 1;
		} else if (0x3f == *from) {      
			to[i] = cpu_to_le16(0xf03f);
			charlen = 1;
		} else if (0x7c== *from) {       
			to[i] = cpu_to_le16(0xf07c);
			charlen = 1;
		} else if (0x3a== *from) {       
			to[i] = cpu_to_le16(0xf022);
			charlen = 1;
		} else if (0x22== *from) {       
			to[i] = cpu_to_le16(0xf020);
			charlen = 1;
		} else {
#endif
		charlen = codepage->char2uni(from, len, &wchar_to);
		if (charlen < 1) {
#ifndef MY_ABC_HERE
			cERROR(1, "strtoUCS: char2uni of 0x%x returned %d",
				*from, charlen);
#endif
			 
			wchar_to = 0x003f;
			charlen = 1;
		}
		put_unaligned_le16(wchar_to, &to[i]);
#ifdef MY_ABC_HERE
		}
#endif
	}

	put_unaligned_le16(0, &to[i]);
	return i;
}

char *
cifs_strndup_from_ucs(const char *src, const int maxlen, const bool is_unicode,
	     const struct nls_table *codepage)
{
	int len;
	char *dst;

	if (is_unicode) {
		len = cifs_ucs2_bytes((__le16 *) src, maxlen, codepage);
		len += nls_nullsize(codepage);
		dst = kmalloc(len, GFP_KERNEL);
		if (!dst)
			return NULL;
		cifs_from_ucs2(dst, (__le16 *) src, len, maxlen, codepage,
			       false);
	} else {
		len = strnlen(src, maxlen);
		len++;
		dst = kmalloc(len, GFP_KERNEL);
		if (!dst)
			return NULL;
		strlcpy(dst, src, len);
	}

	return dst;
}

int
cifsConvertToUCS(__le16 *target, const char *source, int srclen,
		 const struct nls_table *cp, int mapChars)
{
	int i, j, charlen;
	char src_char;
	__le16 dst_char;
	wchar_t tmp;

	if (!mapChars)
		return cifs_strtoUCS(target, source, PATH_MAX, cp);

	for (i = 0, j = 0; i < srclen; j++) {
		src_char = source[i];
		charlen = 1;
		switch (src_char) {
		case 0:
			put_unaligned(0, &target[j]);
			goto ctoUCS_out;
		case ':':
			dst_char = cpu_to_le16(UNI_COLON);
			break;
		case '*':
			dst_char = cpu_to_le16(UNI_ASTERISK);
			break;
		case '?':
			dst_char = cpu_to_le16(UNI_QUESTION);
			break;
		case '<':
			dst_char = cpu_to_le16(UNI_LESSTHAN);
			break;
		case '>':
			dst_char = cpu_to_le16(UNI_GRTRTHAN);
			break;
		case '|':
			dst_char = cpu_to_le16(UNI_PIPE);
			break;
#ifdef MY_ABC_HERE
		case '"':
			dst_char = cpu_to_le16(UNI_DQUOT);
			break;
		case '/':
			dst_char = cpu_to_le16(UNI_DIVSLASH);
			break;
		case '\r':
			dst_char = cpu_to_le16(UNI_CRGRET);
			break;
#endif
		 
		default:
			charlen = cp->char2uni(source + i, srclen - i, &tmp);
			dst_char = cpu_to_le16(tmp);

			if (charlen < 1) {
				dst_char = cpu_to_le16(0x003f);
				charlen = 1;
			}
		}
		 
		i += charlen;
		put_unaligned(dst_char, &target[j]);
	}

ctoUCS_out:
	return j;
}
