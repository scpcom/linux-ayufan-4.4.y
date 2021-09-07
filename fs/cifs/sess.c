#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include "cifspdu.h"
#include "cifsglob.h"
#include "cifsproto.h"
#include "cifs_unicode.h"
#include "cifs_debug.h"
#include "ntlmssp.h"
#include "nterr.h"
#include <linux/utsname.h>
#include "cifs_spnego.h"

extern void SMBNTencrypt(unsigned char *passwd, unsigned char *c8,
			 unsigned char *p24);

static bool is_first_ses_reconnect(struct cifsSesInfo *ses)
{
	struct list_head *tmp;
	struct cifsSesInfo *tmp_ses;

	list_for_each(tmp, &ses->server->smb_ses_list) {
		tmp_ses = list_entry(tmp, struct cifsSesInfo,
				     smb_ses_list);
		if (tmp_ses->need_reconnect == false)
			return false;
	}
	 
	return true;
}

static __le16 get_next_vcnum(struct cifsSesInfo *ses)
{
	__u16 vcnum = 0;
	struct list_head *tmp;
	struct cifsSesInfo *tmp_ses;
	__u16 max_vcs = ses->server->max_vcs;
	__u16 i;
	int free_vc_found = 0;

	if (max_vcs < 2)
		max_vcs = 0xFFFF;

	write_lock(&cifs_tcp_ses_lock);
	if ((ses->need_reconnect) && is_first_ses_reconnect(ses))
			goto get_vc_num_exit;   
	for (i = ses->server->srv_count - 1; i < max_vcs; i++) {
		if (i == 0)  
			break;

		free_vc_found = 1;

		list_for_each(tmp, &ses->server->smb_ses_list) {
			tmp_ses = list_entry(tmp, struct cifsSesInfo,
					     smb_ses_list);
			if (tmp_ses->vcnum == i) {
				free_vc_found = 0;
				break;  
			}
		}
		if (free_vc_found)
			break;  
	}

	if (i == 0)
		vcnum = 0;  
	else if (free_vc_found == 0)
		vcnum = 1;   
	else
		vcnum = i;
	ses->vcnum = vcnum;
get_vc_num_exit:
	write_unlock(&cifs_tcp_ses_lock);

	return cpu_to_le16(vcnum);
}

static __u32 cifs_ssetup_hdr(struct cifsSesInfo *ses, SESSION_SETUP_ANDX *pSMB)
{
	__u32 capabilities = 0;

	pSMB->req.AndXCommand = 0xFF;
	pSMB->req.MaxBufferSize = cpu_to_le16(ses->server->maxBuf);
	pSMB->req.MaxMpxCount = cpu_to_le16(ses->server->maxReq);
	pSMB->req.VcNumber = get_next_vcnum(ses);

	capabilities = CAP_LARGE_FILES | CAP_NT_SMBS | CAP_LEVEL_II_OPLOCKS |
			CAP_LARGE_WRITE_X | CAP_LARGE_READ_X;

	if (ses->server->secMode &
	    (SECMODE_SIGN_REQUIRED | SECMODE_SIGN_ENABLED))
		pSMB->req.hdr.Flags2 |= SMBFLG2_SECURITY_SIGNATURE;

	if (ses->capabilities & CAP_UNICODE) {
		pSMB->req.hdr.Flags2 |= SMBFLG2_UNICODE;
		capabilities |= CAP_UNICODE;
	}
	if (ses->capabilities & CAP_STATUS32) {
		pSMB->req.hdr.Flags2 |= SMBFLG2_ERR_STATUS;
		capabilities |= CAP_STATUS32;
	}
	if (ses->capabilities & CAP_DFS) {
		pSMB->req.hdr.Flags2 |= SMBFLG2_DFS;
		capabilities |= CAP_DFS;
	}
	if (ses->capabilities & CAP_UNIX)
		capabilities |= CAP_UNIX;

	return capabilities;
}

static void
unicode_oslm_strings(char **pbcc_area, const struct nls_table *nls_cp)
{
	char *bcc_ptr = *pbcc_area;
	int bytes_ret = 0;

#ifdef MY_ABC_HERE
	bytes_ret = cifs_strtoUCS((__le16 *)bcc_ptr, "Synology Linux version ", 32,
				  nls_cp);
#else
	bytes_ret = cifs_strtoUCS((__le16 *)bcc_ptr, "Linux version ", 32,
				  nls_cp);
#endif
	bcc_ptr += 2 * bytes_ret;
	bytes_ret = cifs_strtoUCS((__le16 *) bcc_ptr, init_utsname()->release,
				  32, nls_cp);
	bcc_ptr += 2 * bytes_ret;
	bcc_ptr += 2;  

	bytes_ret = cifs_strtoUCS((__le16 *) bcc_ptr, CIFS_NETWORK_OPSYS,
				  32, nls_cp);
	bcc_ptr += 2 * bytes_ret;
	bcc_ptr += 2;  

	*pbcc_area = bcc_ptr;
}

static void unicode_domain_string(char **pbcc_area, struct cifsSesInfo *ses,
				   const struct nls_table *nls_cp)
{
	char *bcc_ptr = *pbcc_area;
	int bytes_ret = 0;

	if (ses->domainName == NULL) {
		 
		*bcc_ptr = 0;
		*(bcc_ptr+1) = 0;
		bytes_ret = 0;
	} else
		bytes_ret = cifs_strtoUCS((__le16 *) bcc_ptr, ses->domainName,
					  256, nls_cp);
	bcc_ptr += 2 * bytes_ret;
	bcc_ptr += 2;   

	*pbcc_area = bcc_ptr;
}

static void unicode_ssetup_strings(char **pbcc_area, struct cifsSesInfo *ses,
				   const struct nls_table *nls_cp)
{
	char *bcc_ptr = *pbcc_area;
	int bytes_ret = 0;

	if (ses->userName == NULL) {
		 
		*bcc_ptr = 0;
		*(bcc_ptr+1) = 0;
	} else {  
		bytes_ret = cifs_strtoUCS((__le16 *) bcc_ptr, ses->userName,
					  300, nls_cp);
	}
	bcc_ptr += 2 * bytes_ret;
	bcc_ptr += 2;  

	unicode_domain_string(&bcc_ptr, ses, nls_cp);
	unicode_oslm_strings(&bcc_ptr, nls_cp);

	*pbcc_area = bcc_ptr;
}

static void ascii_ssetup_strings(char **pbcc_area, struct cifsSesInfo *ses,
				 const struct nls_table *nls_cp)
{
	char *bcc_ptr = *pbcc_area;

	if (ses->userName == NULL) {
		 
	} else {  
		strncpy(bcc_ptr, ses->userName, 300);
	}
	 
	bcc_ptr += strnlen(ses->userName, 300);
	*bcc_ptr = 0;
	bcc_ptr++;  

	if (ses->domainName != NULL) {
		strncpy(bcc_ptr, ses->domainName, 256);
		bcc_ptr += strnlen(ses->domainName, 256);
	}  
	*bcc_ptr = 0;
	bcc_ptr++;

#ifdef MY_ABC_HERE
	strcpy(bcc_ptr, "Synology Linux version ");
	bcc_ptr += strlen("Synology Linux version ");
#else
	strcpy(bcc_ptr, "Linux version ");
	bcc_ptr += strlen("Linux version ");
#endif
	strcpy(bcc_ptr, init_utsname()->release);
	bcc_ptr += strlen(init_utsname()->release) + 1;

	strcpy(bcc_ptr, CIFS_NETWORK_OPSYS);
	bcc_ptr += strlen(CIFS_NETWORK_OPSYS) + 1;

	*pbcc_area = bcc_ptr;
}

static void
decode_unicode_ssetup(char **pbcc_area, int bleft, struct cifsSesInfo *ses,
		      const struct nls_table *nls_cp)
{
	int len;
	char *data = *pbcc_area;

	cFYI(1, ("bleft %d", bleft));

	if (bleft % 2) {
		data[bleft] = 0;
		++bleft;
	}

	kfree(ses->serverOS);
	ses->serverOS = cifs_strndup_from_ucs(data, bleft, true, nls_cp);
	cFYI(1, ("serverOS=%s", ses->serverOS));
	len = (UniStrnlen((wchar_t *) data, bleft / 2) * 2) + 2;
	data += len;
	bleft -= len;
	if (bleft <= 0)
		return;

	kfree(ses->serverNOS);
	ses->serverNOS = cifs_strndup_from_ucs(data, bleft, true, nls_cp);
	cFYI(1, ("serverNOS=%s", ses->serverNOS));
	len = (UniStrnlen((wchar_t *) data, bleft / 2) * 2) + 2;
	data += len;
	bleft -= len;
	if (bleft <= 0)
		return;

	kfree(ses->serverDomain);
	ses->serverDomain = cifs_strndup_from_ucs(data, bleft, true, nls_cp);
	cFYI(1, ("serverDomain=%s", ses->serverDomain));

	return;
}

static int decode_ascii_ssetup(char **pbcc_area, int bleft,
			       struct cifsSesInfo *ses,
			       const struct nls_table *nls_cp)
{
	int rc = 0;
	int len;
	char *bcc_ptr = *pbcc_area;

	cFYI(1, ("decode sessetup ascii. bleft %d", bleft));

	len = strnlen(bcc_ptr, bleft);
	if (len >= bleft)
		return rc;

	kfree(ses->serverOS);

	ses->serverOS = kzalloc(len + 1, GFP_KERNEL);
	if (ses->serverOS)
		strncpy(ses->serverOS, bcc_ptr, len);
	if (strncmp(ses->serverOS, "OS/2", 4) == 0) {
			cFYI(1, ("OS/2 server"));
			ses->flags |= CIFS_SES_OS2;
	}

	bcc_ptr += len + 1;
	bleft -= len + 1;

	len = strnlen(bcc_ptr, bleft);
	if (len >= bleft)
		return rc;

	kfree(ses->serverNOS);

	ses->serverNOS = kzalloc(len + 1, GFP_KERNEL);
	if (ses->serverNOS)
		strncpy(ses->serverNOS, bcc_ptr, len);

	bcc_ptr += len + 1;
	bleft -= len + 1;

	len = strnlen(bcc_ptr, bleft);
	if (len > bleft)
		return rc;

	cFYI(1, ("ascii: bytes left %d", bleft));

	return rc;
}

static int decode_ntlmssp_challenge(char *bcc_ptr, int blob_len,
				    struct cifsSesInfo *ses)
{
	CHALLENGE_MESSAGE *pblob = (CHALLENGE_MESSAGE *)bcc_ptr;

	if (blob_len < sizeof(CHALLENGE_MESSAGE)) {
		cERROR(1, ("challenge blob len %d too small", blob_len));
		return -EINVAL;
	}

	if (memcmp(pblob->Signature, "NTLMSSP", 8)) {
		cERROR(1, ("blob signature incorrect %s", pblob->Signature));
		return -EINVAL;
	}
	if (pblob->MessageType != NtLmChallenge) {
		cERROR(1, ("Incorrect message type %d", pblob->MessageType));
		return -EINVAL;
	}

	memcpy(ses->server->cryptKey, pblob->Challenge, CIFS_CRYPTO_KEY_SIZE);
	 
	return 0;
}

#ifdef CONFIG_CIFS_EXPERIMENTAL
 
static void build_ntlmssp_negotiate_blob(unsigned char *pbuffer,
					 struct cifsSesInfo *ses)
{
	NEGOTIATE_MESSAGE *sec_blob = (NEGOTIATE_MESSAGE *)pbuffer;
	__u32 flags;

	memcpy(sec_blob->Signature, NTLMSSP_SIGNATURE, 8);
	sec_blob->MessageType = NtLmNegotiate;

	flags = NTLMSSP_NEGOTIATE_56 |	NTLMSSP_REQUEST_TARGET |
		NTLMSSP_NEGOTIATE_128 | NTLMSSP_NEGOTIATE_UNICODE |
		NTLMSSP_NEGOTIATE_NT_ONLY | NTLMSSP_NEGOTIATE_NTLM;
	if (ses->server->secMode &
	   (SECMODE_SIGN_REQUIRED | SECMODE_SIGN_ENABLED))
		flags |= NTLMSSP_NEGOTIATE_SIGN;
	if (ses->server->secMode & SECMODE_SIGN_REQUIRED)
		flags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;

	sec_blob->NegotiateFlags |= cpu_to_le32(flags);

	sec_blob->WorkstationName.BufferOffset = 0;
	sec_blob->WorkstationName.Length = 0;
	sec_blob->WorkstationName.MaximumLength = 0;

	sec_blob->DomainName.BufferOffset = 0;
	sec_blob->DomainName.Length = 0;
	sec_blob->DomainName.MaximumLength = 0;
}

static int build_ntlmssp_auth_blob(unsigned char *pbuffer,
				   struct cifsSesInfo *ses,
				   const struct nls_table *nls_cp, int first)
{
	AUTHENTICATE_MESSAGE *sec_blob = (AUTHENTICATE_MESSAGE *)pbuffer;
	__u32 flags;
	unsigned char *tmp;
	char ntlm_session_key[CIFS_SESS_KEY_SIZE];

	memcpy(sec_blob->Signature, NTLMSSP_SIGNATURE, 8);
	sec_blob->MessageType = NtLmAuthenticate;

	flags = NTLMSSP_NEGOTIATE_56 |
		NTLMSSP_REQUEST_TARGET | NTLMSSP_NEGOTIATE_TARGET_INFO |
		NTLMSSP_NEGOTIATE_128 | NTLMSSP_NEGOTIATE_UNICODE |
		NTLMSSP_NEGOTIATE_NT_ONLY | NTLMSSP_NEGOTIATE_NTLM;
	if (ses->server->secMode &
	   (SECMODE_SIGN_REQUIRED | SECMODE_SIGN_ENABLED))
		flags |= NTLMSSP_NEGOTIATE_SIGN;
	if (ses->server->secMode & SECMODE_SIGN_REQUIRED)
		flags |= NTLMSSP_NEGOTIATE_ALWAYS_SIGN;

	tmp = pbuffer + sizeof(AUTHENTICATE_MESSAGE);
	sec_blob->NegotiateFlags |= cpu_to_le32(flags);

	sec_blob->LmChallengeResponse.BufferOffset =
				cpu_to_le32(sizeof(AUTHENTICATE_MESSAGE));
	sec_blob->LmChallengeResponse.Length = 0;
	sec_blob->LmChallengeResponse.MaximumLength = 0;

	SMBNTencrypt(ses->password, ses->server->cryptKey, ntlm_session_key);
	if (first)
		cifs_calculate_mac_key(&ses->server->mac_signing_key,
				       ntlm_session_key, ses->password);

	memcpy(tmp, ntlm_session_key, CIFS_SESS_KEY_SIZE);
	sec_blob->NtChallengeResponse.BufferOffset = cpu_to_le32(tmp - pbuffer);
	sec_blob->NtChallengeResponse.Length = cpu_to_le16(CIFS_SESS_KEY_SIZE);
	sec_blob->NtChallengeResponse.MaximumLength =
				cpu_to_le16(CIFS_SESS_KEY_SIZE);

	tmp += CIFS_SESS_KEY_SIZE;

	if (ses->domainName == NULL) {
		sec_blob->DomainName.BufferOffset = cpu_to_le32(tmp - pbuffer);
		sec_blob->DomainName.Length = 0;
		sec_blob->DomainName.MaximumLength = 0;
		tmp += 2;
	} else {
		int len;
		len = cifs_strtoUCS((__le16 *)tmp, ses->domainName,
				    MAX_USERNAME_SIZE, nls_cp);
		len *= 2;  
		len += 2;  
		sec_blob->DomainName.BufferOffset = cpu_to_le32(tmp - pbuffer);
		sec_blob->DomainName.Length = cpu_to_le16(len);
		sec_blob->DomainName.MaximumLength = cpu_to_le16(len);
		tmp += len;
	}

	if (ses->userName == NULL) {
		sec_blob->UserName.BufferOffset = cpu_to_le32(tmp - pbuffer);
		sec_blob->UserName.Length = 0;
		sec_blob->UserName.MaximumLength = 0;
		tmp += 2;
	} else {
		int len;
		len = cifs_strtoUCS((__le16 *)tmp, ses->userName,
				    MAX_USERNAME_SIZE, nls_cp);
		len *= 2;  
		len += 2;  
		sec_blob->UserName.BufferOffset = cpu_to_le32(tmp - pbuffer);
		sec_blob->UserName.Length = cpu_to_le16(len);
		sec_blob->UserName.MaximumLength = cpu_to_le16(len);
		tmp += len;
	}

	sec_blob->WorkstationName.BufferOffset = cpu_to_le32(tmp - pbuffer);
	sec_blob->WorkstationName.Length = 0;
	sec_blob->WorkstationName.MaximumLength = 0;
	tmp += 2;

	sec_blob->SessionKey.BufferOffset = cpu_to_le32(tmp - pbuffer);
	sec_blob->SessionKey.Length = 0;
	sec_blob->SessionKey.MaximumLength = 0;
	return tmp - pbuffer;
}

static void setup_ntlmssp_neg_req(SESSION_SETUP_ANDX *pSMB,
				 struct cifsSesInfo *ses)
{
	build_ntlmssp_negotiate_blob(&pSMB->req.SecurityBlob[0], ses);
	pSMB->req.SecurityBlobLength = cpu_to_le16(sizeof(NEGOTIATE_MESSAGE));

	return;
}

static int setup_ntlmssp_auth_req(SESSION_SETUP_ANDX *pSMB,
				  struct cifsSesInfo *ses,
				  const struct nls_table *nls, int first_time)
{
	int bloblen;

	bloblen = build_ntlmssp_auth_blob(&pSMB->req.SecurityBlob[0], ses, nls,
					  first_time);
	pSMB->req.SecurityBlobLength = cpu_to_le16(bloblen);

	return bloblen;
}
#endif

int
CIFS_SessSetup(unsigned int xid, struct cifsSesInfo *ses, int first_time,
		const struct nls_table *nls_cp)
{
	int rc = 0;
	int wct;
	struct smb_hdr *smb_buf;
	char *bcc_ptr;
	char *str_area;
	SESSION_SETUP_ANDX *pSMB;
	__u32 capabilities;
	int count;
	int resp_buf_type;
	struct kvec iov[3];
	enum securityEnum type;
	__u16 action;
	int bytes_remaining;
	struct key *spnego_key = NULL;
	__le32 phase = NtLmNegotiate;  

	if (ses == NULL)
		return -EINVAL;

	type = ses->server->secType;

	cFYI(1, ("sess setup type %d", type));
ssetup_ntlmssp_authenticate:
	if (phase == NtLmChallenge)
		phase = NtLmAuthenticate;  

	if (type == LANMAN) {
#ifndef CONFIG_CIFS_WEAK_PW_HASH
		 
		return -EOPNOTSUPP;
#endif
		wct = 10;  
	} else if ((type == NTLM) || (type == NTLMv2)) {
		 
		wct = 13;  
	} else  
		wct = 12;

	rc = small_smb_init_no_tc(SMB_COM_SESSION_SETUP_ANDX, wct, ses,
			    (void **)&smb_buf);
	if (rc)
		return rc;

	pSMB = (SESSION_SETUP_ANDX *)smb_buf;

	capabilities = cifs_ssetup_hdr(ses, pSMB);

	iov[0].iov_base = (char *)pSMB;
	iov[0].iov_len = smb_buf->smb_buf_length + 4;

	resp_buf_type = CIFS_SMALL_BUFFER;

	str_area = kmalloc(2000, GFP_KERNEL);
	if (str_area == NULL) {
		rc = -ENOMEM;
		goto ssetup_exit;
	}
	bcc_ptr = str_area;

	ses->flags &= ~CIFS_SES_LANMAN;

	iov[1].iov_base = NULL;
	iov[1].iov_len = 0;

	if (type == LANMAN) {
#ifdef CONFIG_CIFS_WEAK_PW_HASH
		char lnm_session_key[CIFS_SESS_KEY_SIZE];

		pSMB->req.hdr.Flags2 &= ~SMBFLG2_UNICODE;

		pSMB->old_req.PasswordLength = cpu_to_le16(CIFS_SESS_KEY_SIZE);
		 
		calc_lanman_hash(ses->password, ses->server->cryptKey,
				 ses->server->secMode & SECMODE_PW_ENCRYPT ?
					true : false, lnm_session_key);

		ses->flags |= CIFS_SES_LANMAN;
		memcpy(bcc_ptr, (char *)lnm_session_key, CIFS_SESS_KEY_SIZE);
		bcc_ptr += CIFS_SESS_KEY_SIZE;

		cFYI(1, ("Negotiating LANMAN setting up strings"));
		 
		ascii_ssetup_strings(&bcc_ptr, ses, nls_cp);
#endif
	} else if (type == NTLM) {
		char ntlm_session_key[CIFS_SESS_KEY_SIZE];

		pSMB->req_no_secext.Capabilities = cpu_to_le32(capabilities);
		pSMB->req_no_secext.CaseInsensitivePasswordLength =
			cpu_to_le16(CIFS_SESS_KEY_SIZE);
		pSMB->req_no_secext.CaseSensitivePasswordLength =
			cpu_to_le16(CIFS_SESS_KEY_SIZE);

		SMBNTencrypt(ses->password, ses->server->cryptKey,
			     ntlm_session_key);

		if (first_time)  
			cifs_calculate_mac_key(&ses->server->mac_signing_key,
				ntlm_session_key, ses->password);
		 
		memcpy(bcc_ptr, (char *)ntlm_session_key, CIFS_SESS_KEY_SIZE);
		bcc_ptr += CIFS_SESS_KEY_SIZE;
		memcpy(bcc_ptr, (char *)ntlm_session_key, CIFS_SESS_KEY_SIZE);
		bcc_ptr += CIFS_SESS_KEY_SIZE;
		if (ses->capabilities & CAP_UNICODE) {
			 
			if (iov[0].iov_len % 2) {
				*bcc_ptr = 0;
				bcc_ptr++;
			}
			unicode_ssetup_strings(&bcc_ptr, ses, nls_cp);
		} else
			ascii_ssetup_strings(&bcc_ptr, ses, nls_cp);
	} else if (type == NTLMv2) {
		char *v2_sess_key =
			kmalloc(sizeof(struct ntlmv2_resp), GFP_KERNEL);

		if (v2_sess_key == NULL) {
			rc = -ENOMEM;
			goto ssetup_exit;
		}

		pSMB->req_no_secext.Capabilities = cpu_to_le32(capabilities);

		pSMB->req_no_secext.CaseInsensitivePasswordLength = 0;
		 
		pSMB->req_no_secext.CaseSensitivePasswordLength =
			cpu_to_le16(sizeof(struct ntlmv2_resp));

		setup_ntlmv2_rsp(ses, v2_sess_key, nls_cp);
		if (first_time)  
		 
		memcpy(bcc_ptr, (char *)v2_sess_key,
		       sizeof(struct ntlmv2_resp));
		bcc_ptr += sizeof(struct ntlmv2_resp);
		kfree(v2_sess_key);
		if (ses->capabilities & CAP_UNICODE) {
			if (iov[0].iov_len % 2) {
				*bcc_ptr = 0;
				bcc_ptr++;
			}
			unicode_ssetup_strings(&bcc_ptr, ses, nls_cp);
		} else
			ascii_ssetup_strings(&bcc_ptr, ses, nls_cp);
	} else if (type == Kerberos || type == MSKerberos) {
#ifdef CONFIG_CIFS_UPCALL
		struct cifs_spnego_msg *msg;
		spnego_key = cifs_get_spnego_key(ses);
		if (IS_ERR(spnego_key)) {
			rc = PTR_ERR(spnego_key);
			spnego_key = NULL;
			goto ssetup_exit;
		}

		msg = spnego_key->payload.data;
		 
		if (msg->version != CIFS_SPNEGO_UPCALL_VERSION) {
			cERROR(1, ("incorrect version of cifs.upcall (expected"
				   " %d but got %d)",
				   CIFS_SPNEGO_UPCALL_VERSION, msg->version));
			rc = -EKEYREJECTED;
			goto ssetup_exit;
		}
		 
		if (msg->sesskey_len >
		    sizeof(ses->server->mac_signing_key.data.krb5)) {
			cERROR(1, ("Kerberos signing key too long (%u bytes)",
				msg->sesskey_len));
			rc = -EOVERFLOW;
			goto ssetup_exit;
		}
		if (first_time) {
			ses->server->mac_signing_key.len = msg->sesskey_len;
			memcpy(ses->server->mac_signing_key.data.krb5,
				msg->data, msg->sesskey_len);
		}
		pSMB->req.hdr.Flags2 |= SMBFLG2_EXT_SEC;
		capabilities |= CAP_EXTENDED_SECURITY;
		pSMB->req.Capabilities = cpu_to_le32(capabilities);
		iov[1].iov_base = msg->data + msg->sesskey_len;
		iov[1].iov_len = msg->secblob_len;
		pSMB->req.SecurityBlobLength = cpu_to_le16(iov[1].iov_len);

		if (ses->capabilities & CAP_UNICODE) {
			 
			if ((iov[0].iov_len + iov[1].iov_len) % 2) {
				*bcc_ptr = 0;
				bcc_ptr++;
			}
			unicode_oslm_strings(&bcc_ptr, nls_cp);
			unicode_domain_string(&bcc_ptr, ses, nls_cp);
		} else
		 
			ascii_ssetup_strings(&bcc_ptr, ses, nls_cp);
#else  
		cERROR(1, ("Kerberos negotiated but upcall support disabled!"));
		rc = -ENOSYS;
		goto ssetup_exit;
#endif  
	} else {
#ifdef CONFIG_CIFS_EXPERIMENTAL
		if (type == RawNTLMSSP) {
			if ((pSMB->req.hdr.Flags2 & SMBFLG2_UNICODE) == 0) {
				cERROR(1, ("NTLMSSP requires Unicode support"));
				rc = -ENOSYS;
				goto ssetup_exit;
			}

			cFYI(1, ("ntlmssp session setup phase %d", phase));
			pSMB->req.hdr.Flags2 |= SMBFLG2_EXT_SEC;
			capabilities |= CAP_EXTENDED_SECURITY;
			pSMB->req.Capabilities |= cpu_to_le32(capabilities);
			if (phase == NtLmNegotiate) {
				setup_ntlmssp_neg_req(pSMB, ses);
				iov[1].iov_len = sizeof(NEGOTIATE_MESSAGE);
			} else if (phase == NtLmAuthenticate) {
				int blob_len;
				blob_len = setup_ntlmssp_auth_req(pSMB, ses,
								  nls_cp,
								  first_time);
				iov[1].iov_len = blob_len;
				 
				smb_buf->Uid = ses->Suid;
			} else {
				cERROR(1, ("invalid phase %d", phase));
				rc = -ENOSYS;
				goto ssetup_exit;
			}
			iov[1].iov_base = &pSMB->req.SecurityBlob[0];
			 
			if ((iov[0].iov_len + iov[1].iov_len) % 2) {
				*bcc_ptr = 0;
				bcc_ptr++;
			}
			unicode_oslm_strings(&bcc_ptr, nls_cp);
		} else {
			cERROR(1, ("secType %d not supported!", type));
			rc = -ENOSYS;
			goto ssetup_exit;
		}
#else
		cERROR(1, ("secType %d not supported!", type));
		rc = -ENOSYS;
		goto ssetup_exit;
#endif
	}

	iov[2].iov_base = str_area;
	iov[2].iov_len = (long) bcc_ptr - (long) str_area;

	count = iov[1].iov_len + iov[2].iov_len;
	smb_buf->smb_buf_length += count;

	BCC_LE(smb_buf) = cpu_to_le16(count);

	rc = SendReceive2(xid, ses, iov, 3  , &resp_buf_type,
			  CIFS_STD_OP   | CIFS_LOG_ERROR);
	 
	cFYI(1, ("ssetup rc from sendrecv2 is %d", rc));

	pSMB = (SESSION_SETUP_ANDX *)iov[0].iov_base;
	smb_buf = (struct smb_hdr *)iov[0].iov_base;

	if ((type == RawNTLMSSP) && (smb_buf->Status.CifsError ==
			cpu_to_le32(NT_STATUS_MORE_PROCESSING_REQUIRED))) {
		if (phase != NtLmNegotiate) {
			cERROR(1, ("Unexpected more processing error"));
			goto ssetup_exit;
		}
		 
		phase = NtLmChallenge;  
		rc = 0;  
	}
	if (rc)
		goto ssetup_exit;

	if ((smb_buf->WordCount != 3) && (smb_buf->WordCount != 4)) {
		rc = -EIO;
		cERROR(1, ("bad word count %d", smb_buf->WordCount));
		goto ssetup_exit;
	}
	action = le16_to_cpu(pSMB->resp.Action);
	if (action & GUEST_LOGIN)
		cFYI(1, ("Guest login"));  
	ses->Suid = smb_buf->Uid;    
	cFYI(1, ("UID = %d ", ses->Suid));
	 
	bytes_remaining = BCC(smb_buf);
	bcc_ptr = pByteArea(smb_buf);

	if (smb_buf->WordCount == 4) {
		__u16 blob_len;
		blob_len = le16_to_cpu(pSMB->resp.SecurityBlobLength);
		if (blob_len > bytes_remaining) {
			cERROR(1, ("bad security blob length %d", blob_len));
			rc = -EINVAL;
			goto ssetup_exit;
		}
		if (phase == NtLmChallenge) {
			rc = decode_ntlmssp_challenge(bcc_ptr, blob_len, ses);
			 
			if (rc)
				goto ssetup_exit;
		}
		bcc_ptr += blob_len;
		bytes_remaining -= blob_len;
	}

	if (smb_buf->Flags2 & SMBFLG2_UNICODE) {
		 
		if (((unsigned long) bcc_ptr - (unsigned long) smb_buf) % 2) {
			++bcc_ptr;
			--bytes_remaining;
		}
		decode_unicode_ssetup(&bcc_ptr, bytes_remaining, ses, nls_cp);
	} else {
		rc = decode_ascii_ssetup(&bcc_ptr, bytes_remaining,
					 ses, nls_cp);
	}

ssetup_exit:
	if (spnego_key) {
		key_revoke(spnego_key);
		key_put(spnego_key);
	}
	kfree(str_area);
	if (resp_buf_type == CIFS_SMALL_BUFFER) {
		cFYI(1, ("ssetup freeing small buf %p", iov[0].iov_base));
		cifs_small_buf_release(iov[0].iov_base);
	} else if (resp_buf_type == CIFS_LARGE_BUFFER)
		cifs_buf_release(iov[0].iov_base);

	if ((phase == NtLmChallenge) && (rc == 0))
		goto ssetup_ntlmssp_authenticate;

	return rc;
}
