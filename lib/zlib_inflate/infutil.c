#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
#include <linux/zutil.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>

#if defined (MY_DEF_HERE)
static int zlib_inflate_data(void *unzip_buf, unsigned int sz,
		      const void *buf, unsigned int len, int header)
#else  
 
int zlib_inflate_blob(void *gunzip_buf, unsigned int sz,
		      const void *buf, unsigned int len)
#endif  
{
	const u8 *zbuf = buf;
	struct z_stream_s *strm;
	int rc;
#if defined (MY_DEF_HERE)
	int windowBits = header ? MAX_WBITS : -MAX_WBITS;
#endif  

	rc = -ENOMEM;
	strm = kmalloc(sizeof(*strm), GFP_KERNEL);
	if (strm == NULL)
		goto gunzip_nomem1;
	strm->workspace = kmalloc(zlib_inflate_workspacesize(), GFP_KERNEL);
	if (strm->workspace == NULL)
		goto gunzip_nomem2;

	strm->next_in = zbuf;
	strm->avail_in = len;
#if defined (MY_DEF_HERE)
	strm->next_out = unzip_buf;
	strm->avail_out = sz;

	rc = zlib_inflateInit2(strm, windowBits);
#else  
	strm->next_out = gunzip_buf;
	strm->avail_out = sz;

	rc = zlib_inflateInit2(strm, -MAX_WBITS);
#endif  
	if (rc == Z_OK) {
		rc = zlib_inflate(strm, Z_FINISH);
		 
		if (rc == Z_STREAM_END)
			rc = sz - strm->avail_out;
		else
			rc = -EINVAL;
		zlib_inflateEnd(strm);
	} else
		rc = -EINVAL;

	kfree(strm->workspace);
gunzip_nomem2:
	kfree(strm);
gunzip_nomem1:
	return rc;  
}
#if defined (MY_DEF_HERE)

int zlib_inflate_blob(void *gunzip_buf, unsigned int sz,
		      const void *buf, unsigned int len)
{
	 
	return zlib_inflate_data(gunzip_buf, sz, buf, len, 0);
}

#ifdef CONFIG_ST_ELF_EXTENSIONS
 
int zlib_inflate_blob_with_header(void *unzip_buf, unsigned int sz,
				  const void *buf, unsigned int len)
{
	return zlib_inflate_data(unzip_buf, sz, buf, len, 1);
}
#endif  
#endif  