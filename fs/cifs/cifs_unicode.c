#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
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

	for (i = 0; i < maxwords && from[i]; i++) {
		charlen = codepage->uni2char(le16_to_cpu(from[i]), tmp,
					     NLS_MAX_CHARSET_SIZE);
		if (charlen > 0)
			outlen += charlen;
		else
			outlen++;
	}

	return outlen;
}

static int
cifs_mapchar(char *target, const __le16 src_char, const struct nls_table *cp,
	     bool mapchar)
{
	int len = 1;

	if (!mapchar)
		goto cp_convert;

	switch (le16_to_cpu(src_char)) {
	case UNI_COLON:
		*target = ':';
		break;
	case UNI_ASTERIK:
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
	default:
		goto cp_convert;
	}

out:
	return len;

cp_convert:
	len = cp->uni2char(le16_to_cpu(src_char), target,
			   NLS_MAX_CHARSET_SIZE);
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

	safelen = tolen - (NLS_MAX_CHARSET_SIZE + nullsize);

	for (i = 0; i < fromwords && from[i]; i++) {
		 
		if (outlen >= safelen) {
			charlen = cifs_mapchar(tmp, from[i], codepage, mapchar);
			if ((outlen + charlen) > (tolen - nullsize))
				break;
		}

		charlen = cifs_mapchar(&to[outlen], from[i], codepage, mapchar);
		outlen += charlen;
	}

	for (i = 0; i < nullsize; i++)
		to[outlen++] = 0;

	return outlen;
}

int
cifs_strtoUCS(__le16 *to, const char *from, int len,
	      const struct nls_table *codepage)
{
	int charlen;
	int i;
	wchar_t *wchar_to = (wchar_t *)to;  

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
		 
		charlen = codepage->char2uni(from, len, &wchar_to[i]);
		if (charlen < 1) {
#ifndef MY_ABC_HERE
			cERROR(1,
			       ("strtoUCS: char2uni of %d returned %d",
				(int)*from, charlen));
#endif
			 
			to[i] = cpu_to_le16(0x003f);
			charlen = 1;
		} else
			to[i] = cpu_to_le16(wchar_to[i]);

#ifdef MY_ABC_HERE
		}
#endif
	}

	to[i] = 0;
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
