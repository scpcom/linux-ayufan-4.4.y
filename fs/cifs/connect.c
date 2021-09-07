#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/net.h>
#include <linux/string.h>
#include <linux/list.h>
#include <linux/wait.h>
#include <linux/pagemap.h>
#include <linux/ctype.h>
#include <linux/utsname.h>
#include <linux/mempool.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/kthread.h>
#include <linux/pagevec.h>
#include <linux/freezer.h>
#include <linux/namei.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <linux/inet.h>
#include <net/ipv6.h>
#include "cifspdu.h"
#include "cifsglob.h"
#include "cifsproto.h"
#include "cifs_unicode.h"
#include "cifs_debug.h"
#include "cifs_fs_sb.h"
#include "ntlmssp.h"
#include "nterr.h"
#include "rfc1002pdu.h"
#include "cn_cifs.h"

#define CIFS_PORT 445
#define RFC1001_PORT 139

extern void SMBNTencrypt(unsigned char *passwd, unsigned char *c8,
			 unsigned char *p24);

extern mempool_t *cifs_req_poolp;

struct smb_vol {
	char *username;
	char *password;
	char *domainname;
	char *UNC;
	char *UNCip;
	char *iocharset;   
	char source_rfc1001_name[16];  
	char target_rfc1001_name[16];  
	uid_t linux_uid;
	gid_t linux_gid;
	mode_t file_mode;
	mode_t dir_mode;
	unsigned secFlg;
	bool retry:1;
	bool intr:1;
	bool setuids:1;
	bool override_uid:1;
	bool override_gid:1;
	bool dynperm:1;
	bool noperm:1;
	bool no_psx_acl:1;  
	bool cifs_acl:1;
	bool no_xattr:1;    
	bool server_ino:1;  
	bool direct_io:1;
	bool remap:1;       
	bool posix_paths:1;  
	bool no_linux_ext:1;
	bool sfu_emul:1;
	bool nullauth:1;    
	bool nocase:1;      
	bool nobrl:1;       
	bool mand_lock:1;   
	bool seal:1;        
	bool nodfs:1;       
	bool local_lease:1;  
	bool noblocksnd:1;
	bool noautotune:1;
	bool nostrictsync:1;  
	unsigned int rsize;
	unsigned int wsize;
	unsigned int sockopt;
	unsigned short int port;
	char *prepath;
};

static int ipv4_connect(struct TCP_Server_Info *server);
static int ipv6_connect(struct TCP_Server_Info *server);

static int
cifs_reconnect(struct TCP_Server_Info *server)
{
	int rc = 0;
	struct list_head *tmp, *tmp2;
	struct cifsSesInfo *ses;
	struct cifsTconInfo *tcon;
	struct mid_q_entry *mid_entry;

	spin_lock(&GlobalMid_Lock);
	if (server->tcpStatus == CifsExiting) {
		 
		spin_unlock(&GlobalMid_Lock);
		return rc;
	} else
		server->tcpStatus = CifsNeedReconnect;
	spin_unlock(&GlobalMid_Lock);
	server->maxBuf = 0;

	cFYI(1, ("Reconnecting tcp session"));

	read_lock(&cifs_tcp_ses_lock);
	list_for_each(tmp, &server->smb_ses_list) {
		ses = list_entry(tmp, struct cifsSesInfo, smb_ses_list);
		ses->need_reconnect = true;
		ses->ipc_tid = 0;
		list_for_each(tmp2, &ses->tcon_list) {
			tcon = list_entry(tmp2, struct cifsTconInfo, tcon_list);
			tcon->need_reconnect = true;
		}
	}
	read_unlock(&cifs_tcp_ses_lock);
	 
	mutex_lock(&server->srv_mutex);
	if (server->ssocket) {
		cFYI(1, ("State: 0x%x Flags: 0x%lx", server->ssocket->state,
			server->ssocket->flags));
		kernel_sock_shutdown(server->ssocket, SHUT_WR);
		cFYI(1, ("Post shutdown state: 0x%x Flags: 0x%lx",
			server->ssocket->state,
			server->ssocket->flags));
		sock_release(server->ssocket);
		server->ssocket = NULL;
	}

	spin_lock(&GlobalMid_Lock);
	list_for_each(tmp, &server->pending_mid_q) {
		mid_entry = list_entry(tmp, struct
					mid_q_entry,
					qhead);
		if (mid_entry->midState == MID_REQUEST_SUBMITTED) {
				 
			mid_entry->midState = MID_RETRY_NEEDED;
		}
	}
	spin_unlock(&GlobalMid_Lock);
	mutex_unlock(&server->srv_mutex);

	while ((server->tcpStatus != CifsExiting) &&
	       (server->tcpStatus != CifsGood)) {
		try_to_freeze();
		if (server->addr.sockAddr6.sin6_family == AF_INET6)
			rc = ipv6_connect(server);
		else
			rc = ipv4_connect(server);
		if (rc) {
			cFYI(1, ("reconnect error %d", rc));
			msleep(3000);
		} else {
			atomic_inc(&tcpSesReconnectCount);
			spin_lock(&GlobalMid_Lock);
			if (server->tcpStatus != CifsExiting)
				server->tcpStatus = CifsGood;
			server->sequence_number = 0;
			spin_unlock(&GlobalMid_Lock);
	 
			wake_up(&server->response_q);
		}
	}
	return rc;
}

static int check2ndT2(struct smb_hdr *pSMB, unsigned int maxBufSize)
{
	struct smb_t2_rsp *pSMBt;
	int total_data_size;
	int data_in_this_rsp;
	int remaining;

	if (pSMB->Command != SMB_COM_TRANSACTION2)
		return 0;

	if (pSMB->WordCount != 10) {  
		cFYI(1, ("invalid transact2 word count"));
		return -EINVAL;
	}

	pSMBt = (struct smb_t2_rsp *)pSMB;

	total_data_size = le16_to_cpu(pSMBt->t2_rsp.TotalDataCount);
	data_in_this_rsp = le16_to_cpu(pSMBt->t2_rsp.DataCount);

	remaining = total_data_size - data_in_this_rsp;

	if (remaining == 0)
		return 0;
	else if (remaining < 0) {
		cFYI(1, ("total data %d smaller than data in frame %d",
			total_data_size, data_in_this_rsp));
		return -EINVAL;
	} else {
		cFYI(1, ("missing %d bytes from transact2, check next response",
			remaining));
		if (total_data_size > maxBufSize) {
			cERROR(1, ("TotalDataSize %d is over maximum buffer %d",
				total_data_size, maxBufSize));
			return -EINVAL;
		}
		return remaining;
	}
}

static int coalesce_t2(struct smb_hdr *psecond, struct smb_hdr *pTargetSMB)
{
	struct smb_t2_rsp *pSMB2 = (struct smb_t2_rsp *)psecond;
	struct smb_t2_rsp *pSMBt  = (struct smb_t2_rsp *)pTargetSMB;
	int total_data_size;
	int total_in_buf;
	int remaining;
	int total_in_buf2;
	char *data_area_of_target;
	char *data_area_of_buf2;
	__u16 byte_count;

	total_data_size = le16_to_cpu(pSMBt->t2_rsp.TotalDataCount);

	if (total_data_size != le16_to_cpu(pSMB2->t2_rsp.TotalDataCount)) {
		cFYI(1, ("total data size of primary and secondary t2 differ"));
	}

	total_in_buf = le16_to_cpu(pSMBt->t2_rsp.DataCount);

	remaining = total_data_size - total_in_buf;

	if (remaining < 0)
		return -EINVAL;

	if (remaining == 0)  
		return 0;

	total_in_buf2 = le16_to_cpu(pSMB2->t2_rsp.DataCount);
	if (remaining < total_in_buf2) {
		cFYI(1, ("transact2 2nd response contains too much data"));
	}

	data_area_of_target = (char *)&pSMBt->hdr.Protocol +
				le16_to_cpu(pSMBt->t2_rsp.DataOffset);
	 
	data_area_of_buf2 = (char *) &pSMB2->hdr.Protocol +
					le16_to_cpu(pSMB2->t2_rsp.DataOffset);

	data_area_of_target += total_in_buf;

	memcpy(data_area_of_target, data_area_of_buf2, total_in_buf2);
	total_in_buf += total_in_buf2;
	pSMBt->t2_rsp.DataCount = cpu_to_le16(total_in_buf);
	byte_count = le16_to_cpu(BCC_LE(pTargetSMB));
	byte_count += total_in_buf2;
	BCC_LE(pTargetSMB) = cpu_to_le16(byte_count);

	byte_count = pTargetSMB->smb_buf_length;
	byte_count += total_in_buf2;

	pTargetSMB->smb_buf_length = byte_count;

	if (remaining == total_in_buf2) {
		cFYI(1, ("found the last secondary response"));
		return 0;  
	} else  
		return 1;

}

static int
cifs_demultiplex_thread(struct TCP_Server_Info *server)
{
	int length;
	unsigned int pdu_length, total_read;
	struct smb_hdr *smb_buffer = NULL;
	struct smb_hdr *bigbuf = NULL;
	struct smb_hdr *smallbuf = NULL;
	struct msghdr smb_msg;
	struct kvec iov;
	struct socket *csocket = server->ssocket;
	struct list_head *tmp;
	struct cifsSesInfo *ses;
	struct task_struct *task_to_wake = NULL;
	struct mid_q_entry *mid_entry;
	char temp;
	bool isLargeBuf = false;
	bool isMultiRsp;
	int reconnect;

	current->flags |= PF_MEMALLOC;
	cFYI(1, ("Demultiplex PID: %d", task_pid_nr(current)));

	length = atomic_inc_return(&tcpSesAllocCount);
	if (length > 1)
		mempool_resize(cifs_req_poolp, length + cifs_min_rcv,
				GFP_KERNEL);

	set_freezable();
	while (server->tcpStatus != CifsExiting) {
		if (try_to_freeze())
			continue;
		if (bigbuf == NULL) {
			bigbuf = cifs_buf_get();
			if (!bigbuf) {
				cERROR(1, ("No memory for large SMB response"));
				msleep(3000);
				 
				continue;
			}
		} else if (isLargeBuf) {
			 
			memset(bigbuf, 0, sizeof(struct smb_hdr));
		}

		if (smallbuf == NULL) {
			smallbuf = cifs_small_buf_get();
			if (!smallbuf) {
				cERROR(1, ("No memory for SMB response"));
				msleep(1000);
				 
				continue;
			}
			 
		} else  
			memset(smallbuf, 0, sizeof(struct smb_hdr));

		isLargeBuf = false;
		isMultiRsp = false;
		smb_buffer = smallbuf;
		iov.iov_base = smb_buffer;
		iov.iov_len = 4;
		smb_msg.msg_control = NULL;
		smb_msg.msg_controllen = 0;
		pdu_length = 4;  
incomplete_rcv:
		length =
		    kernel_recvmsg(csocket, &smb_msg,
				&iov, 1, pdu_length, 0  );

		if (server->tcpStatus == CifsExiting) {
			break;
		} else if (server->tcpStatus == CifsNeedReconnect) {
			cFYI(1, ("Reconnect after server stopped responding"));
			cifs_reconnect(server);
			cFYI(1, ("call to reconnect done"));
			csocket = server->ssocket;
			continue;
		} else if ((length == -ERESTARTSYS) || (length == -EAGAIN)) {
			msleep(1);  
			if (pdu_length < 4) {
				iov.iov_base = (4 - pdu_length) +
							(char *)smb_buffer;
				iov.iov_len = pdu_length;
				smb_msg.msg_control = NULL;
				smb_msg.msg_controllen = 0;
				goto incomplete_rcv;
			} else
				continue;
		} else if (length <= 0) {
			if (server->tcpStatus == CifsNew) {
				cFYI(1, ("tcp session abend after SMBnegprot"));
				 
				break;
			}
			if (!try_to_freeze() && (length == -EINTR)) {
				cFYI(1, ("cifsd thread killed"));
				break;
			}
			cFYI(1, ("Reconnect after unexpected peek error %d",
				length));
			cifs_reconnect(server);
			csocket = server->ssocket;
			wake_up(&server->response_q);
			continue;
		} else if (length < pdu_length) {
			cFYI(1, ("requested %d bytes but only got %d bytes",
				  pdu_length, length));
			pdu_length -= length;
			msleep(1);
			goto incomplete_rcv;
		}

		temp = *((char *) smb_buffer);

		pdu_length = be32_to_cpu((__force __be32)smb_buffer->smb_buf_length);
		smb_buffer->smb_buf_length = pdu_length;

		cFYI(1, ("rfc1002 length 0x%x", pdu_length+4));

		if (temp == (char) RFC1002_SESSION_KEEP_ALIVE) {
			continue;
		} else if (temp == (char)RFC1002_POSITIVE_SESSION_RESPONSE) {
			cFYI(1, ("Good RFC 1002 session rsp"));
			continue;
		} else if (temp == (char)RFC1002_NEGATIVE_SESSION_RESPONSE) {
			 
			cFYI(1, ("Negative RFC1002 Session Response Error 0x%x)",
				pdu_length));
			if (server->tcpStatus == CifsNew) {
				 
				break;
			} else {
				 
				msleep(1000);
				 
				server->addr.sockAddr.sin_port =
					htons(CIFS_PORT);
				cifs_reconnect(server);
				csocket = server->ssocket;
				wake_up(&server->response_q);
				continue;
			}
		} else if (temp != (char) 0) {
			cERROR(1, ("Unknown RFC 1002 frame"));
			cifs_dump_mem(" Received Data: ", (char *)smb_buffer,
				      length);
			cifs_reconnect(server);
			csocket = server->ssocket;
			continue;
		}

		if ((pdu_length > CIFSMaxBufSize + MAX_CIFS_HDR_SIZE - 4) ||
			    (pdu_length < sizeof(struct smb_hdr) - 1 - 4)) {
			cERROR(1, ("Invalid size SMB length %d pdu_length %d",
					length, pdu_length+4));
			cifs_reconnect(server);
			csocket = server->ssocket;
			wake_up(&server->response_q);
			continue;
		}

		reconnect = 0;

		if (pdu_length > MAX_CIFS_SMALL_BUFFER_SIZE - 4) {
			isLargeBuf = true;
			memcpy(bigbuf, smallbuf, 4);
			smb_buffer = bigbuf;
		}
		length = 0;
		iov.iov_base = 4 + (char *)smb_buffer;
		iov.iov_len = pdu_length;
		for (total_read = 0; total_read < pdu_length;
		     total_read += length) {
			length = kernel_recvmsg(csocket, &smb_msg, &iov, 1,
						pdu_length - total_read, 0);
			if ((server->tcpStatus == CifsExiting) ||
			    (length == -EINTR)) {
				 
				reconnect = 2;
				break;
			} else if (server->tcpStatus == CifsNeedReconnect) {
				cifs_reconnect(server);
				csocket = server->ssocket;
				 
				reconnect = 1;
				break;
			} else if ((length == -ERESTARTSYS) ||
				   (length == -EAGAIN)) {
				msleep(1);  
				length = 0;
				continue;
			} else if (length <= 0) {
				cERROR(1, ("Received no data, expecting %d",
					      pdu_length - total_read));
				cifs_reconnect(server);
				csocket = server->ssocket;
				reconnect = 1;
				break;
			}
		}
		if (reconnect == 2)
			break;
		else if (reconnect == 1)
			continue;

		length += 4;  

		dump_smb(smb_buffer, length);
		if (checkSMB(smb_buffer, smb_buffer->Mid, total_read+4)) {
			cifs_dump_mem("Bad SMB: ", smb_buffer, 48);
			continue;
		}

		task_to_wake = NULL;
		spin_lock(&GlobalMid_Lock);
		list_for_each(tmp, &server->pending_mid_q) {
			mid_entry = list_entry(tmp, struct mid_q_entry, qhead);

			if ((mid_entry->mid == smb_buffer->Mid) &&
			    (mid_entry->midState == MID_REQUEST_SUBMITTED) &&
			    (mid_entry->command == smb_buffer->Command)) {
				if (check2ndT2(smb_buffer,server->maxBuf) > 0) {
					 
					isMultiRsp = true;
					if (mid_entry->resp_buf) {
						 
						if (coalesce_t2(smb_buffer,
							mid_entry->resp_buf)) {
							mid_entry->multiRsp =
								 true;
							break;
						} else {
							 
							mid_entry->multiEnd =
								 true;
							goto multi_t2_fnd;
						}
					} else {
						if (!isLargeBuf) {
							cERROR(1,("1st trans2 resp needs bigbuf"));
					 
						} else {
							 
							mid_entry->resp_buf =
								 smb_buffer;
							mid_entry->largeBuf =
								 true;
							bigbuf = NULL;
						}
					}
					break;
				}
				mid_entry->resp_buf = smb_buffer;
				mid_entry->largeBuf = isLargeBuf;
multi_t2_fnd:
				task_to_wake = mid_entry->tsk;
				mid_entry->midState = MID_RESPONSE_RECEIVED;
#ifdef CONFIG_CIFS_STATS2
				mid_entry->when_received = jiffies;
#endif
				 
				server->lstrp = jiffies;
				break;
			}
		}
		spin_unlock(&GlobalMid_Lock);
		if (task_to_wake) {
			 
			if (!isMultiRsp) {
				 
				if (isLargeBuf)
					bigbuf = NULL;
				else
					smallbuf = NULL;
			}
			wake_up_process(task_to_wake);
		} else if (!is_valid_oplock_break(smb_buffer, server) &&
			   !isMultiRsp) {
			cERROR(1, ("No task to wake, unknown frame received! "
				   "NumMids %d", midCount.counter));
			cifs_dump_mem("Received Data is: ", (char *)smb_buffer,
				      sizeof(struct smb_hdr));
#ifdef CONFIG_CIFS_DEBUG2
			cifs_dump_detail(smb_buffer);
			cifs_dump_mids(server);
#endif  

		}
	}  

	write_lock(&cifs_tcp_ses_lock);
	list_del_init(&server->tcp_ses_list);
	write_unlock(&cifs_tcp_ses_lock);

	spin_lock(&GlobalMid_Lock);
	server->tcpStatus = CifsExiting;
	spin_unlock(&GlobalMid_Lock);
	wake_up_all(&server->response_q);

	spin_lock(&GlobalMid_Lock);
	if (atomic_read(&server->inFlight) >= cifs_max_pending)
		atomic_set(&server->inFlight, cifs_max_pending - 1);
	 
	spin_unlock(&GlobalMid_Lock);
	 
	wake_up_all(&server->request_q);
	 
	msleep(125);

	if (server->ssocket) {
		sock_release(csocket);
		server->ssocket = NULL;
	}
	 
	cifs_buf_release(bigbuf);
	if (smallbuf)  
		cifs_small_buf_release(smallbuf);

	read_lock(&cifs_tcp_ses_lock);
	if (list_empty(&server->pending_mid_q)) {
		 
		list_for_each(tmp, &server->smb_ses_list) {
			ses = list_entry(tmp, struct cifsSesInfo,
					 smb_ses_list);
			ses->status = CifsExiting;
			ses->server = NULL;
		}
		read_unlock(&cifs_tcp_ses_lock);
	} else {
		 
		list_for_each(tmp, &server->smb_ses_list) {
			ses = list_entry(tmp, struct cifsSesInfo,
					 smb_ses_list);
			ses->status = CifsExiting;
		}

		spin_lock(&GlobalMid_Lock);
		list_for_each(tmp, &server->pending_mid_q) {
		mid_entry = list_entry(tmp, struct mid_q_entry, qhead);
			if (mid_entry->midState == MID_REQUEST_SUBMITTED) {
				cFYI(1, ("Clearing Mid 0x%x - waking up ",
					 mid_entry->mid));
				task_to_wake = mid_entry->tsk;
				if (task_to_wake)
					wake_up_process(task_to_wake);
			}
		}
		spin_unlock(&GlobalMid_Lock);
		read_unlock(&cifs_tcp_ses_lock);
		 
		msleep(125);
	}

	if (!list_empty(&server->pending_mid_q)) {
		 
		cFYI(1, ("Wait for exit from demultiplex thread"));
		msleep(46000);
		 
	}

	read_lock(&cifs_tcp_ses_lock);
	list_for_each(tmp, &server->smb_ses_list) {
		ses = list_entry(tmp, struct cifsSesInfo, smb_ses_list);
		ses->server = NULL;
	}
	read_unlock(&cifs_tcp_ses_lock);

	kfree(server->hostname);
	task_to_wake = xchg(&server->tsk, NULL);
	kfree(server);

	length = atomic_dec_return(&tcpSesAllocCount);
	if (length  > 0)
		mempool_resize(cifs_req_poolp, length + cifs_min_rcv,
				GFP_KERNEL);

	if (!task_to_wake) {
		set_current_state(TASK_INTERRUPTIBLE);
		while (!signal_pending(current)) {
			schedule();
			set_current_state(TASK_INTERRUPTIBLE);
		}
		set_current_state(TASK_RUNNING);
	}

	module_put_and_exit(0);
}

static char *
extract_hostname(const char *unc)
{
	const char *src;
	char *dst, *delim;
	unsigned int len;

	src = unc + 2;

	delim = strchr(src, '\\');
	if (!delim)
		return ERR_PTR(-EINVAL);

	len = delim - src;
	dst = kmalloc((len + 1), GFP_KERNEL);
	if (dst == NULL)
		return ERR_PTR(-ENOMEM);

	memcpy(dst, src, len);
	dst[len] = '\0';

	return dst;
}

static int
cifs_parse_mount_options(char *options, const char *devname,
			 struct smb_vol *vol)
{
	char *value;
	char *data;
	unsigned int  temp_len, i, j;
	char separator[2];
	short int override_uid = -1;
	short int override_gid = -1;
	bool uid_specified = false;
	bool gid_specified = false;

	separator[0] = ',';
	separator[1] = 0;

	if (Local_System_Name[0] != 0)
		memcpy(vol->source_rfc1001_name, Local_System_Name, 15);
	else {
		char *nodename = utsname()->nodename;
		int n = strnlen(nodename, 15);
		memset(vol->source_rfc1001_name, 0x20, 15);
		for (i = 0; i < n; i++) {
			 
			vol->source_rfc1001_name[i] = toupper(nodename[i]);
		}
	}
	vol->source_rfc1001_name[15] = 0;
	 
	vol->target_rfc1001_name[0] = 0;
	vol->linux_uid = current_uid();   
	vol->linux_gid = current_gid();

	vol->dir_mode = vol->file_mode = S_IRUGO | S_IXUGO | S_IWUSR;

	vol->posix_paths = 1;
	 
	vol->server_ino = 1;

	if (!options)
		return 1;

	if (strncmp(options, "sep=", 4) == 0) {
		if (options[4] != 0) {
			separator[0] = options[4];
			options += 5;
		} else {
			cFYI(1, ("Null separator not allowed"));
		}
	}

	while ((data = strsep(&options, separator)) != NULL) {
		if (!*data)
			continue;
		if ((value = strchr(data, '=')) != NULL)
			*value++ = '\0';

		if (strnicmp(data, "user_xattr", 10) == 0) {
			vol->no_xattr = 0;
		} else if (strnicmp(data, "nouser_xattr", 12) == 0) {
			vol->no_xattr = 1;
		} else if (strnicmp(data, "user", 4) == 0) {
			if (!value) {
				printk(KERN_WARNING
				       "CIFS: invalid or missing username\n");
				return 1;	 
			} else if (!*value) {
				 
				vol->nullauth = 1;
			}
			if (strnlen(value, 200) < 200) {
				vol->username = value;
			} else {
				printk(KERN_WARNING "CIFS: username too long\n");
				return 1;
			}
		} else if (strnicmp(data, "pass", 4) == 0) {
			if (!value) {
				vol->password = NULL;
				continue;
			} else if (value[0] == 0) {
				 
				if (value[1] != separator[0]) {
					vol->password = NULL;
					continue;
				}
			}
			temp_len = strlen(value);
			 
			if ((value[temp_len] == 0) &&
			    (value[temp_len+1] == separator[0])) {
				 
				value[temp_len] = separator[0];
				temp_len += 2;   
				while (value[temp_len] != 0)  {
					if (value[temp_len] == separator[0]) {
						if (value[temp_len+1] ==
						     separator[0]) {
						 
							temp_len++;
						} else {
						 
							break;
						}
					}
					temp_len++;
				}
				if (value[temp_len] == 0) {
					options = NULL;
				} else {
					value[temp_len] = 0;
					 
					options = value + temp_len + 1;
				}
				 
				vol->password = kzalloc(temp_len, GFP_KERNEL);
				if (vol->password == NULL) {
					printk(KERN_WARNING "CIFS: no memory "
							    "for password\n");
					return 1;
				}
				for (i = 0, j = 0; i < temp_len; i++, j++) {
					vol->password[j] = value[i];
					if (value[i] == separator[0]
						&& value[i+1] == separator[0]) {
						 
						i++;
					}
				}
				vol->password[j] = 0;
			} else {
				vol->password = kzalloc(temp_len+1, GFP_KERNEL);
				if (vol->password == NULL) {
					printk(KERN_WARNING "CIFS: no memory "
							    "for password\n");
					return 1;
				}
				strcpy(vol->password, value);
			}
		} else if (!strnicmp(data, "ip", 2) ||
			   !strnicmp(data, "addr", 4)) {
			if (!value || !*value) {
				vol->UNCip = NULL;
			} else if (strnlen(value, INET6_ADDRSTRLEN) <
							INET6_ADDRSTRLEN) {
				vol->UNCip = value;
			} else {
				printk(KERN_WARNING "CIFS: ip address "
						    "too long\n");
				return 1;
			}
		} else if (strnicmp(data, "sec", 3) == 0) {
			if (!value || !*value) {
				cERROR(1, ("no security value specified"));
				continue;
			} else if (strnicmp(value, "krb5i", 5) == 0) {
				vol->secFlg |= CIFSSEC_MAY_KRB5 |
					CIFSSEC_MUST_SIGN;
			} else if (strnicmp(value, "krb5p", 5) == 0) {
				 
				cERROR(1, ("Krb5 cifs privacy not supported"));
				return 1;
			} else if (strnicmp(value, "krb5", 4) == 0) {
				vol->secFlg |= CIFSSEC_MAY_KRB5;
#ifdef CONFIG_CIFS_EXPERIMENTAL
			} else if (strnicmp(value, "ntlmsspi", 8) == 0) {
				vol->secFlg |= CIFSSEC_MAY_NTLMSSP |
					CIFSSEC_MUST_SIGN;
			} else if (strnicmp(value, "ntlmssp", 7) == 0) {
				vol->secFlg |= CIFSSEC_MAY_NTLMSSP;
#endif
			} else if (strnicmp(value, "ntlmv2i", 7) == 0) {
				vol->secFlg |= CIFSSEC_MAY_NTLMV2 |
					CIFSSEC_MUST_SIGN;
			} else if (strnicmp(value, "ntlmv2", 6) == 0) {
				vol->secFlg |= CIFSSEC_MAY_NTLMV2;
			} else if (strnicmp(value, "ntlmi", 5) == 0) {
				vol->secFlg |= CIFSSEC_MAY_NTLM |
					CIFSSEC_MUST_SIGN;
			} else if (strnicmp(value, "ntlm", 4) == 0) {
				 
				vol->secFlg |= CIFSSEC_MAY_NTLM;
			} else if (strnicmp(value, "nontlm", 6) == 0) {
				 
				vol->secFlg |= CIFSSEC_MAY_NTLMV2;
#ifdef CONFIG_CIFS_WEAK_PW_HASH
			} else if (strnicmp(value, "lanman", 6) == 0) {
				vol->secFlg |= CIFSSEC_MAY_LANMAN;
#endif
			} else if (strnicmp(value, "none", 4) == 0) {
				vol->nullauth = 1;
			} else {
				cERROR(1, ("bad security option: %s", value));
				return 1;
			}
		} else if ((strnicmp(data, "unc", 3) == 0)
			   || (strnicmp(data, "target", 6) == 0)
			   || (strnicmp(data, "path", 4) == 0)) {
			if (!value || !*value) {
				printk(KERN_WARNING "CIFS: invalid path to "
						    "network resource\n");
				return 1;	 
			}
			if ((temp_len = strnlen(value, 300)) < 300) {
				vol->UNC = kmalloc(temp_len+1, GFP_KERNEL);
				if (vol->UNC == NULL)
					return 1;
				strcpy(vol->UNC, value);
				if (strncmp(vol->UNC, "//", 2) == 0) {
					vol->UNC[0] = '\\';
					vol->UNC[1] = '\\';
				} else if (strncmp(vol->UNC, "\\\\", 2) != 0) {
					printk(KERN_WARNING
					       "CIFS: UNC Path does not begin "
					       "with // or \\\\ \n");
					return 1;
				}
			} else {
				printk(KERN_WARNING "CIFS: UNC name too long\n");
				return 1;
			}
		} else if ((strnicmp(data, "domain", 3) == 0)
			   || (strnicmp(data, "workgroup", 5) == 0)) {
			if (!value || !*value) {
				printk(KERN_WARNING "CIFS: invalid domain name\n");
				return 1;	 
			}
			 
			if (strnlen(value, 256) < 256) {
				vol->domainname = value;
				cFYI(1, ("Domain name set"));
			} else {
				printk(KERN_WARNING "CIFS: domain name too "
						    "long\n");
				return 1;
			}
		} else if (strnicmp(data, "prefixpath", 10) == 0) {
			if (!value || !*value) {
				printk(KERN_WARNING
					"CIFS: invalid path prefix\n");
				return 1;        
			}
			if ((temp_len = strnlen(value, 1024)) < 1024) {
				if (value[0] != '/')
					temp_len++;   
				vol->prepath = kmalloc(temp_len+1, GFP_KERNEL);
				if (vol->prepath == NULL)
					return 1;
				if (value[0] != '/') {
					vol->prepath[0] = '/';
					strcpy(vol->prepath+1, value);
				} else
					strcpy(vol->prepath, value);
				cFYI(1, ("prefix path %s", vol->prepath));
			} else {
				printk(KERN_WARNING "CIFS: prefix too long\n");
				return 1;
			}
		} else if (strnicmp(data, "iocharset", 9) == 0) {
			if (!value || !*value) {
				printk(KERN_WARNING "CIFS: invalid iocharset "
						    "specified\n");
				return 1;	 
			}
			if (strnlen(value, 65) < 65) {
				if (strnicmp(value, "default", 7))
					vol->iocharset = value;
				 
				cFYI(1, ("iocharset set to %s", value));
			} else {
				printk(KERN_WARNING "CIFS: iocharset name "
						    "too long.\n");
				return 1;
			}
		} else if (!strnicmp(data, "uid", 3) && value && *value) {
			vol->linux_uid = simple_strtoul(value, &value, 0);
			uid_specified = true;
		} else if (!strnicmp(data, "forceuid", 8)) {
			override_uid = 1;
		} else if (!strnicmp(data, "noforceuid", 10)) {
			override_uid = 0;
		} else if (!strnicmp(data, "gid", 3) && value && *value) {
			vol->linux_gid = simple_strtoul(value, &value, 0);
			gid_specified = true;
		} else if (!strnicmp(data, "forcegid", 8)) {
			override_gid = 1;
		} else if (!strnicmp(data, "noforcegid", 10)) {
			override_gid = 0;
		} else if (strnicmp(data, "file_mode", 4) == 0) {
			if (value && *value) {
				vol->file_mode =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "dir_mode", 4) == 0) {
			if (value && *value) {
				vol->dir_mode =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "dirmode", 4) == 0) {
			if (value && *value) {
				vol->dir_mode =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "port", 4) == 0) {
			if (value && *value) {
				vol->port =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "rsize", 5) == 0) {
			if (value && *value) {
				vol->rsize =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "wsize", 5) == 0) {
			if (value && *value) {
				vol->wsize =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "sockopt", 5) == 0) {
			if (value && *value) {
				vol->sockopt =
					simple_strtoul(value, &value, 0);
			}
		} else if (strnicmp(data, "netbiosname", 4) == 0) {
			if (!value || !*value || (*value == ' ')) {
				cFYI(1, ("invalid (empty) netbiosname"));
			} else {
				memset(vol->source_rfc1001_name, 0x20, 15);
				for (i = 0; i < 15; i++) {
				 
					if (value[i] == 0)
						break;
					else
						vol->source_rfc1001_name[i] =
								value[i];
				}
				 
				if ((i == 15) && (value[i] != 0))
					printk(KERN_WARNING "CIFS: netbiosname"
						" longer than 15 truncated.\n");
			}
		} else if (strnicmp(data, "servern", 7) == 0) {
			 
			if (!value || !*value || (*value == ' ')) {
				cFYI(1, ("empty server netbiosname specified"));
			} else {
				 
				memset(vol->target_rfc1001_name, 0x20, 16);

				for (i = 0; i < 15; i++) {
				 
					if (value[i] == 0)
						break;
					else
						vol->target_rfc1001_name[i] =
								value[i];
				}
				 
				if ((i == 15) && (value[i] != 0))
					printk(KERN_WARNING "CIFS: server net"
					"biosname longer than 15 truncated.\n");
			}
		} else if (strnicmp(data, "credentials", 4) == 0) {
			 
		} else if (strnicmp(data, "version", 3) == 0) {
			 
		} else if (strnicmp(data, "guest", 5) == 0) {
			 
		} else if (strnicmp(data, "rw", 2) == 0) {
			 
		} else if (strnicmp(data, "ro", 2) == 0) {
			 
		} else if (strnicmp(data, "noblocksend", 11) == 0) {
			vol->noblocksnd = 1;
		} else if (strnicmp(data, "noautotune", 10) == 0) {
			vol->noautotune = 1;
		} else if ((strnicmp(data, "suid", 4) == 0) ||
				   (strnicmp(data, "nosuid", 6) == 0) ||
				   (strnicmp(data, "exec", 4) == 0) ||
				   (strnicmp(data, "noexec", 6) == 0) ||
				   (strnicmp(data, "nodev", 5) == 0) ||
				   (strnicmp(data, "noauto", 6) == 0) ||
				   (strnicmp(data, "dev", 3) == 0)) {
			 
			continue;
		} else if (strnicmp(data, "hard", 4) == 0) {
			vol->retry = 1;
		} else if (strnicmp(data, "soft", 4) == 0) {
			vol->retry = 0;
		} else if (strnicmp(data, "perm", 4) == 0) {
			vol->noperm = 0;
		} else if (strnicmp(data, "noperm", 6) == 0) {
			vol->noperm = 1;
		} else if (strnicmp(data, "mapchars", 8) == 0) {
			vol->remap = 1;
		} else if (strnicmp(data, "nomapchars", 10) == 0) {
			vol->remap = 0;
		} else if (strnicmp(data, "sfu", 3) == 0) {
			vol->sfu_emul = 1;
		} else if (strnicmp(data, "nosfu", 5) == 0) {
			vol->sfu_emul = 0;
		} else if (strnicmp(data, "nodfs", 5) == 0) {
			vol->nodfs = 1;
		} else if (strnicmp(data, "posixpaths", 10) == 0) {
			vol->posix_paths = 1;
		} else if (strnicmp(data, "noposixpaths", 12) == 0) {
			vol->posix_paths = 0;
		} else if (strnicmp(data, "nounix", 6) == 0) {
			vol->no_linux_ext = 1;
		} else if (strnicmp(data, "nolinux", 7) == 0) {
			vol->no_linux_ext = 1;
		} else if ((strnicmp(data, "nocase", 6) == 0) ||
			   (strnicmp(data, "ignorecase", 10)  == 0)) {
#ifdef MY_ABC_HERE
			SynoPosixSemanticsEnabled = 0;
#endif
			vol->nocase = 1;
		} else if (strnicmp(data, "brl", 3) == 0) {
			vol->nobrl =  0;
		} else if ((strnicmp(data, "nobrl", 5) == 0) ||
			   (strnicmp(data, "nolock", 6) == 0)) {
			vol->nobrl =  1;
			 
			if (vol->file_mode ==
				(S_IALLUGO & ~(S_ISUID | S_IXGRP)))
				vol->file_mode = S_IALLUGO;
		} else if (strnicmp(data, "forcemandatorylock", 9) == 0) {
			 
			vol->mand_lock = 1;
		} else if (strnicmp(data, "setuids", 7) == 0) {
			vol->setuids = 1;
		} else if (strnicmp(data, "nosetuids", 9) == 0) {
			vol->setuids = 0;
		} else if (strnicmp(data, "dynperm", 7) == 0) {
			vol->dynperm = true;
		} else if (strnicmp(data, "nodynperm", 9) == 0) {
			vol->dynperm = false;
		} else if (strnicmp(data, "nohard", 6) == 0) {
			vol->retry = 0;
		} else if (strnicmp(data, "nosoft", 6) == 0) {
			vol->retry = 1;
		} else if (strnicmp(data, "nointr", 6) == 0) {
			vol->intr = 0;
		} else if (strnicmp(data, "intr", 4) == 0) {
			vol->intr = 1;
		} else if (strnicmp(data, "nostrictsync", 12) == 0) {
			vol->nostrictsync = 1;
		} else if (strnicmp(data, "strictsync", 10) == 0) {
			vol->nostrictsync = 0;
		} else if (strnicmp(data, "serverino", 7) == 0) {
			vol->server_ino = 1;
		} else if (strnicmp(data, "noserverino", 9) == 0) {
			vol->server_ino = 0;
		} else if (strnicmp(data, "cifsacl", 7) == 0) {
			vol->cifs_acl = 1;
		} else if (strnicmp(data, "nocifsacl", 9) == 0) {
			vol->cifs_acl = 0;
		} else if (strnicmp(data, "acl", 3) == 0) {
			vol->no_psx_acl = 0;
		} else if (strnicmp(data, "noacl", 5) == 0) {
			vol->no_psx_acl = 1;
#ifdef CONFIG_CIFS_EXPERIMENTAL
		} else if (strnicmp(data, "locallease", 6) == 0) {
			vol->local_lease = 1;
#endif
		} else if (strnicmp(data, "sign", 4) == 0) {
			vol->secFlg |= CIFSSEC_MUST_SIGN;
		} else if (strnicmp(data, "seal", 4) == 0) {
			 
			vol->seal = 1;
		} else if (strnicmp(data, "direct", 6) == 0) {
			vol->direct_io = 1;
		} else if (strnicmp(data, "forcedirectio", 13) == 0) {
			vol->direct_io = 1;
		} else if (strnicmp(data, "noac", 4) == 0) {
			printk(KERN_WARNING "CIFS: Mount option noac not "
				"supported. Instead set "
				"/proc/fs/cifs/LookupCacheEnabled to 0\n");
		} else
			printk(KERN_WARNING "CIFS: Unknown mount option %s\n",
						data);
	}
	if (vol->UNC == NULL) {
		if (devname == NULL) {
			printk(KERN_WARNING "CIFS: Missing UNC name for mount "
						"target\n");
			return 1;
		}
		if ((temp_len = strnlen(devname, 300)) < 300) {
			vol->UNC = kmalloc(temp_len+1, GFP_KERNEL);
			if (vol->UNC == NULL)
				return 1;
			strcpy(vol->UNC, devname);
			if (strncmp(vol->UNC, "//", 2) == 0) {
				vol->UNC[0] = '\\';
				vol->UNC[1] = '\\';
			} else if (strncmp(vol->UNC, "\\\\", 2) != 0) {
				printk(KERN_WARNING "CIFS: UNC Path does not "
						    "begin with // or \\\\ \n");
				return 1;
			}
			value = strpbrk(vol->UNC+2, "/\\");
			if (value)
				*value = '\\';
		} else {
			printk(KERN_WARNING "CIFS: UNC name too long\n");
			return 1;
		}
	}
	if (vol->UNCip == NULL)
		vol->UNCip = &vol->UNC[2];

	if (uid_specified)
		vol->override_uid = override_uid;
	else if (override_uid == 1)
		printk(KERN_NOTICE "CIFS: ignoring forceuid mount option "
				   "specified with no uid= option.\n");

	if (gid_specified)
		vol->override_gid = override_gid;
	else if (override_gid == 1)
		printk(KERN_NOTICE "CIFS: ignoring forcegid mount option "
				   "specified with no gid= option.\n");

	return 0;
}

static struct TCP_Server_Info *
cifs_find_tcp_session(struct sockaddr_storage *addr, unsigned short int port)
{
	struct list_head *tmp;
	struct TCP_Server_Info *server;
	struct sockaddr_in *addr4 = (struct sockaddr_in *) addr;
	struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *) addr;

	write_lock(&cifs_tcp_ses_lock);
	list_for_each(tmp, &cifs_tcp_ses_list) {
		server = list_entry(tmp, struct TCP_Server_Info,
				    tcp_ses_list);
		 
		if (server->tcpStatus == CifsNew)
			continue;

		switch (addr->ss_family) {
		case AF_INET:
			if (addr4->sin_addr.s_addr ==
			    server->addr.sockAddr.sin_addr.s_addr) {
				addr4->sin_port = htons(port);
				 
				if (addr4->sin_port) {
					if (addr4->sin_port !=
					    server->addr.sockAddr.sin_port)
						continue;
				}
				break;
			} else
				continue;

		case AF_INET6:
			if (ipv6_addr_equal(&addr6->sin6_addr,
			    &server->addr.sockAddr6.sin6_addr) &&
			    (addr6->sin6_scope_id ==
			    server->addr.sockAddr6.sin6_scope_id)) {
				addr6->sin6_port = htons(port);
				 
				if (addr6->sin6_port) {
					if (addr6->sin6_port !=
					   server->addr.sockAddr6.sin6_port)
						continue;
				}
				break;
			} else
				continue;
		}

		++server->srv_count;
		write_unlock(&cifs_tcp_ses_lock);
		cFYI(1, ("Existing tcp session with server found"));
		return server;
	}
	write_unlock(&cifs_tcp_ses_lock);
	return NULL;
}

static void
cifs_put_tcp_session(struct TCP_Server_Info *server)
{
	struct task_struct *task;

	write_lock(&cifs_tcp_ses_lock);
	if (--server->srv_count > 0) {
		write_unlock(&cifs_tcp_ses_lock);
		return;
	}

	list_del_init(&server->tcp_ses_list);
	write_unlock(&cifs_tcp_ses_lock);

	spin_lock(&GlobalMid_Lock);
	server->tcpStatus = CifsExiting;
	spin_unlock(&GlobalMid_Lock);

	task = xchg(&server->tsk, NULL);
	if (task)
		force_sig(SIGKILL, task);
}

static struct TCP_Server_Info *
cifs_get_tcp_session(struct smb_vol *volume_info)
{
	struct TCP_Server_Info *tcp_ses = NULL;
	struct sockaddr_storage addr;
	struct sockaddr_in *sin_server = (struct sockaddr_in *) &addr;
	struct sockaddr_in6 *sin_server6 = (struct sockaddr_in6 *) &addr;
	int rc;

	memset(&addr, 0, sizeof(struct sockaddr_storage));

	cFYI(1, ("UNC: %s ip: %s", volume_info->UNC, volume_info->UNCip));

	if (volume_info->UNCip && volume_info->UNC) {
		rc = cifs_convert_address(volume_info->UNCip, &addr);
		if (!rc) {
			 
			rc = -EINVAL;
			goto out_err;
		}
	} else if (volume_info->UNCip) {
		 
		cERROR(1, ("Connecting to DFS root not implemented yet"));
		rc = -EINVAL;
		goto out_err;
	} else   {
		cERROR(1,
		       ("CIFS mount error: No UNC path (e.g. -o "
			"unc=//192.168.1.100/public) specified"));
		rc = -EINVAL;
		goto out_err;
	}

	tcp_ses = cifs_find_tcp_session(&addr, volume_info->port);
	if (tcp_ses)
		return tcp_ses;

	tcp_ses = kzalloc(sizeof(struct TCP_Server_Info), GFP_KERNEL);
	if (!tcp_ses) {
		rc = -ENOMEM;
		goto out_err;
	}

	tcp_ses->hostname = extract_hostname(volume_info->UNC);
	if (IS_ERR(tcp_ses->hostname)) {
		rc = PTR_ERR(tcp_ses->hostname);
		goto out_err;
	}

	tcp_ses->noblocksnd = volume_info->noblocksnd;
	tcp_ses->noautotune = volume_info->noautotune;
	atomic_set(&tcp_ses->inFlight, 0);
	init_waitqueue_head(&tcp_ses->response_q);
	init_waitqueue_head(&tcp_ses->request_q);
	INIT_LIST_HEAD(&tcp_ses->pending_mid_q);
	mutex_init(&tcp_ses->srv_mutex);
	memcpy(tcp_ses->workstation_RFC1001_name,
		volume_info->source_rfc1001_name, RFC1001_NAME_LEN_WITH_NULL);
	memcpy(tcp_ses->server_RFC1001_name,
		volume_info->target_rfc1001_name, RFC1001_NAME_LEN_WITH_NULL);
	tcp_ses->sequence_number = 0;
	INIT_LIST_HEAD(&tcp_ses->tcp_ses_list);
	INIT_LIST_HEAD(&tcp_ses->smb_ses_list);

	tcp_ses->tcpStatus = CifsNew;
	++tcp_ses->srv_count;

	if (addr.ss_family == AF_INET6) {
		cFYI(1, ("attempting ipv6 connect"));
		 
		sin_server6->sin6_port = htons(volume_info->port);
		memcpy(&tcp_ses->addr.sockAddr6, sin_server6,
			sizeof(struct sockaddr_in6));
		rc = ipv6_connect(tcp_ses);
	} else {
		sin_server->sin_port = htons(volume_info->port);
		memcpy(&tcp_ses->addr.sockAddr, sin_server,
			sizeof(struct sockaddr_in));
		rc = ipv4_connect(tcp_ses);
	}
	if (rc < 0) {
		cERROR(1, ("Error connecting to socket. Aborting operation"));
		goto out_err;
	}

	__module_get(THIS_MODULE);
	tcp_ses->tsk = kthread_run((void *)(void *)cifs_demultiplex_thread,
				  tcp_ses, "cifsd");
	if (IS_ERR(tcp_ses->tsk)) {
		rc = PTR_ERR(tcp_ses->tsk);
		cERROR(1, ("error %d create cifsd thread", rc));
		module_put(THIS_MODULE);
		goto out_err;
	}

	write_lock(&cifs_tcp_ses_lock);
	list_add(&tcp_ses->tcp_ses_list, &cifs_tcp_ses_list);
	write_unlock(&cifs_tcp_ses_lock);

	return tcp_ses;

out_err:
	if (tcp_ses) {
		if (!IS_ERR(tcp_ses->hostname))
			kfree(tcp_ses->hostname);
		if (tcp_ses->ssocket)
			sock_release(tcp_ses->ssocket);
		kfree(tcp_ses);
	}
	return ERR_PTR(rc);
}

static struct cifsSesInfo *
cifs_find_smb_ses(struct TCP_Server_Info *server, char *username)
{
	struct list_head *tmp;
	struct cifsSesInfo *ses;

	write_lock(&cifs_tcp_ses_lock);
	list_for_each(tmp, &server->smb_ses_list) {
		ses = list_entry(tmp, struct cifsSesInfo, smb_ses_list);
		if (strncmp(ses->userName, username, MAX_USERNAME_SIZE))
			continue;

		++ses->ses_count;
		write_unlock(&cifs_tcp_ses_lock);
		return ses;
	}
	write_unlock(&cifs_tcp_ses_lock);
	return NULL;
}

static void
cifs_put_smb_ses(struct cifsSesInfo *ses)
{
	int xid;
	struct TCP_Server_Info *server = ses->server;

	write_lock(&cifs_tcp_ses_lock);
	if (--ses->ses_count > 0) {
		write_unlock(&cifs_tcp_ses_lock);
		return;
	}

	list_del_init(&ses->smb_ses_list);
	write_unlock(&cifs_tcp_ses_lock);

	if (ses->status == CifsGood) {
		xid = GetXid();
		CIFSSMBLogoff(xid, ses);
		_FreeXid(xid);
	}
	sesInfoFree(ses);
	cifs_put_tcp_session(server);
}

static struct cifsTconInfo *
cifs_find_tcon(struct cifsSesInfo *ses, const char *unc)
{
	struct list_head *tmp;
	struct cifsTconInfo *tcon;

	write_lock(&cifs_tcp_ses_lock);
	list_for_each(tmp, &ses->tcon_list) {
		tcon = list_entry(tmp, struct cifsTconInfo, tcon_list);
		if (tcon->tidStatus == CifsExiting)
			continue;
		if (strncmp(tcon->treeName, unc, MAX_TREE_SIZE))
			continue;

		++tcon->tc_count;
		write_unlock(&cifs_tcp_ses_lock);
		return tcon;
	}
	write_unlock(&cifs_tcp_ses_lock);
	return NULL;
}

static void
cifs_put_tcon(struct cifsTconInfo *tcon)
{
	int xid;
	struct cifsSesInfo *ses = tcon->ses;

	write_lock(&cifs_tcp_ses_lock);
	if (--tcon->tc_count > 0) {
		write_unlock(&cifs_tcp_ses_lock);
		return;
	}

	list_del_init(&tcon->tcon_list);
	write_unlock(&cifs_tcp_ses_lock);

	xid = GetXid();
	CIFSSMBTDis(xid, tcon);
	_FreeXid(xid);

	tconInfoFree(tcon);
	cifs_put_smb_ses(ses);
}

int
get_dfs_path(int xid, struct cifsSesInfo *pSesInfo, const char *old_path,
	     const struct nls_table *nls_codepage, unsigned int *pnum_referrals,
	     struct dfs_info3_param **preferrals, int remap)
{
	char *temp_unc;
	int rc = 0;

	*pnum_referrals = 0;
	*preferrals = NULL;

	if (pSesInfo->ipc_tid == 0) {
		temp_unc = kmalloc(2   +
			strnlen(pSesInfo->serverName,
				SERVER_NAME_LEN_WITH_NULL * 2)
				 + 1 + 4    + 2,
				GFP_KERNEL);
		if (temp_unc == NULL)
			return -ENOMEM;
		temp_unc[0] = '\\';
		temp_unc[1] = '\\';
		strcpy(temp_unc + 2, pSesInfo->serverName);
		strcpy(temp_unc + 2 + strlen(pSesInfo->serverName), "\\IPC$");
		rc = CIFSTCon(xid, pSesInfo, temp_unc, NULL, nls_codepage);
		cFYI(1,
		     ("CIFS Tcon rc = %d ipc_tid = %d", rc, pSesInfo->ipc_tid));
		kfree(temp_unc);
	}
	if (rc == 0)
		rc = CIFSGetDFSRefer(xid, pSesInfo, old_path, preferrals,
				     pnum_referrals, nls_codepage, remap);
	 
	return rc;
}

#ifdef CONFIG_DEBUG_LOCK_ALLOC
static struct lock_class_key cifs_key[2];
static struct lock_class_key cifs_slock_key[2];

static inline void
cifs_reclassify_socket4(struct socket *sock)
{
	struct sock *sk = sock->sk;
	BUG_ON(sock_owned_by_user(sk));
	sock_lock_init_class_and_name(sk, "slock-AF_INET-CIFS",
		&cifs_slock_key[0], "sk_lock-AF_INET-CIFS", &cifs_key[0]);
}

static inline void
cifs_reclassify_socket6(struct socket *sock)
{
	struct sock *sk = sock->sk;
	BUG_ON(sock_owned_by_user(sk));
	sock_lock_init_class_and_name(sk, "slock-AF_INET6-CIFS",
		&cifs_slock_key[1], "sk_lock-AF_INET6-CIFS", &cifs_key[1]);
}
#else
static inline void
cifs_reclassify_socket4(struct socket *sock)
{
}

static inline void
cifs_reclassify_socket6(struct socket *sock)
{
}
#endif

static void rfc1002mangle(char *target, char *source, unsigned int length)
{
	unsigned int i, j;

	for (i = 0, j = 0; i < (length); i++) {
		 
		target[j] = 'A' + (0x0F & (source[i] >> 4));
		target[j+1] = 'A' + (0x0F & source[i]);
		j += 2;
	}

}

static int
ipv4_connect(struct TCP_Server_Info *server)
{
	int rc = 0;
	bool connected = false;
	__be16 orig_port = 0;
	struct socket *socket = server->ssocket;

	if (socket == NULL) {
		rc = sock_create_kern(PF_INET, SOCK_STREAM,
				      IPPROTO_TCP, &socket);
		if (rc < 0) {
			cERROR(1, ("Error %d creating socket", rc));
			return rc;
		}

		cFYI(1, ("Socket created"));
		server->ssocket = socket;
		socket->sk->sk_allocation = GFP_NOFS;
		cifs_reclassify_socket4(socket);
	}

	if (server->addr.sockAddr.sin_port) {
		rc = socket->ops->connect(socket, (struct sockaddr *)
					  &server->addr.sockAddr,
					  sizeof(struct sockaddr_in), 0);
		if (rc >= 0)
			connected = true;
	}

	if (!connected) {
		 
		orig_port = server->addr.sockAddr.sin_port;

		if (server->addr.sockAddr.sin_port != htons(CIFS_PORT)) {
			server->addr.sockAddr.sin_port = htons(CIFS_PORT);
			rc = socket->ops->connect(socket,
						(struct sockaddr *)
						&server->addr.sockAddr,
						sizeof(struct sockaddr_in), 0);
			if (rc >= 0)
				connected = true;
		}
	}
	if (!connected) {
		server->addr.sockAddr.sin_port = htons(RFC1001_PORT);
		rc = socket->ops->connect(socket, (struct sockaddr *)
					      &server->addr.sockAddr,
					      sizeof(struct sockaddr_in), 0);
		if (rc >= 0)
			connected = true;
	}

	if (!connected) {
		if (orig_port)
			server->addr.sockAddr.sin_port = orig_port;
		cFYI(1, ("Error %d connecting to server via ipv4", rc));
		sock_release(socket);
		server->ssocket = NULL;
		return rc;
	}

	socket->sk->sk_rcvtimeo = 7 * HZ;
	socket->sk->sk_sndtimeo = 5 * HZ;

	if (server->noautotune) {
		if (socket->sk->sk_sndbuf < (200 * 1024))
			socket->sk->sk_sndbuf = 200 * 1024;
		if (socket->sk->sk_rcvbuf < (140 * 1024))
			socket->sk->sk_rcvbuf = 140 * 1024;
	}

	 cFYI(1, ("sndbuf %d rcvbuf %d rcvtimeo 0x%lx",
		 socket->sk->sk_sndbuf,
		 socket->sk->sk_rcvbuf, socket->sk->sk_rcvtimeo));

	if (server->addr.sockAddr.sin_port == htons(RFC1001_PORT)) {
		 
		struct rfc1002_session_packet *ses_init_buf;
		struct smb_hdr *smb_buf;
		ses_init_buf = kzalloc(sizeof(struct rfc1002_session_packet),
				       GFP_KERNEL);
		if (ses_init_buf) {
			ses_init_buf->trailer.session_req.called_len = 32;
			if (server->server_RFC1001_name &&
			    server->server_RFC1001_name[0] != 0)
				rfc1002mangle(ses_init_buf->trailer.
						session_req.called_name,
					      server->server_RFC1001_name,
					      RFC1001_NAME_LEN_WITH_NULL);
			else
				rfc1002mangle(ses_init_buf->trailer.
						session_req.called_name,
					      DEFAULT_CIFS_CALLED_NAME,
					      RFC1001_NAME_LEN_WITH_NULL);

			ses_init_buf->trailer.session_req.calling_len = 32;

			if (server->workstation_RFC1001_name &&
			    server->workstation_RFC1001_name[0] != 0)
				rfc1002mangle(ses_init_buf->trailer.
						session_req.calling_name,
					      server->workstation_RFC1001_name,
					      RFC1001_NAME_LEN_WITH_NULL);
			else
				rfc1002mangle(ses_init_buf->trailer.
						session_req.calling_name,
					      "LINUX_CIFS_CLNT",
					      RFC1001_NAME_LEN_WITH_NULL);

			ses_init_buf->trailer.session_req.scope1 = 0;
			ses_init_buf->trailer.session_req.scope2 = 0;
			smb_buf = (struct smb_hdr *)ses_init_buf;
			 
			smb_buf->smb_buf_length = 0x81000044;
			rc = smb_send(server, smb_buf, 0x44);
			kfree(ses_init_buf);
			msleep(1);  
		}
		 
	}

	return rc;
}

static int
ipv6_connect(struct TCP_Server_Info *server)
{
	int rc = 0;
	bool connected = false;
	__be16 orig_port = 0;
	struct socket *socket = server->ssocket;

	if (socket == NULL) {
		rc = sock_create_kern(PF_INET6, SOCK_STREAM,
				      IPPROTO_TCP, &socket);
		if (rc < 0) {
			cERROR(1, ("Error %d creating ipv6 socket", rc));
			socket = NULL;
			return rc;
		}

		cFYI(1, ("ipv6 Socket created"));
		server->ssocket = socket;
		socket->sk->sk_allocation = GFP_NOFS;
		cifs_reclassify_socket6(socket);
	}

	if (server->addr.sockAddr6.sin6_port) {
		rc = socket->ops->connect(socket,
				(struct sockaddr *) &server->addr.sockAddr6,
				sizeof(struct sockaddr_in6), 0);
		if (rc >= 0)
			connected = true;
	}

	if (!connected) {
		 
		orig_port = server->addr.sockAddr6.sin6_port;
		 
		if (server->addr.sockAddr6.sin6_port != htons(CIFS_PORT)) {
			server->addr.sockAddr6.sin6_port = htons(CIFS_PORT);
			rc = socket->ops->connect(socket, (struct sockaddr *)
					&server->addr.sockAddr6,
					sizeof(struct sockaddr_in6), 0);
			if (rc >= 0)
				connected = true;
		}
	}
	if (!connected) {
		server->addr.sockAddr6.sin6_port = htons(RFC1001_PORT);
		rc = socket->ops->connect(socket, (struct sockaddr *)
				&server->addr.sockAddr6,
				sizeof(struct sockaddr_in6), 0);
		if (rc >= 0)
			connected = true;
	}

	if (!connected) {
		if (orig_port)
			server->addr.sockAddr6.sin6_port = orig_port;
		cFYI(1, ("Error %d connecting to server via ipv6", rc));
		sock_release(socket);
		server->ssocket = NULL;
		return rc;
	}

	socket->sk->sk_rcvtimeo = 7 * HZ;
	socket->sk->sk_sndtimeo = 5 * HZ;
	server->ssocket = socket;

	return rc;
}

void reset_cifs_unix_caps(int xid, struct cifsTconInfo *tcon,
			  struct super_block *sb, struct smb_vol *vol_info)
{
	 
	__u64 saved_cap = le64_to_cpu(tcon->fsUnixInfo.Capability);

	if (vol_info && vol_info->no_linux_ext) {
		tcon->fsUnixInfo.Capability = 0;
		tcon->unix_ext = 0;  
		cFYI(1, ("Linux protocol extensions disabled"));
		return;
	} else if (vol_info)
		tcon->unix_ext = 1;  

	if (tcon->unix_ext == 0) {
		cFYI(1, ("Unix extensions disabled so not set on reconnect"));
		return;
	}

	if (!CIFSSMBQFSUnixInfo(xid, tcon)) {
		__u64 cap = le64_to_cpu(tcon->fsUnixInfo.Capability);

		if (vol_info == NULL) {
			 
			if ((saved_cap & CIFS_UNIX_POSIX_ACL_CAP) == 0)
				cap &= ~CIFS_UNIX_POSIX_ACL_CAP;
			if ((saved_cap & CIFS_UNIX_POSIX_PATHNAMES_CAP) == 0) {
				if (cap & CIFS_UNIX_POSIX_PATHNAMES_CAP)
					cERROR(1, ("POSIXPATH support change"));
				cap &= ~CIFS_UNIX_POSIX_PATHNAMES_CAP;
			} else if ((cap & CIFS_UNIX_POSIX_PATHNAMES_CAP) == 0) {
				cERROR(1, ("possible reconnect error"));
				cERROR(1,
					("server disabled POSIX path support"));
			}
		}

		cap &= CIFS_UNIX_CAP_MASK;
		if (vol_info && vol_info->no_psx_acl)
			cap &= ~CIFS_UNIX_POSIX_ACL_CAP;
		else if (CIFS_UNIX_POSIX_ACL_CAP & cap) {
			cFYI(1, ("negotiated posix acl support"));
			if (sb)
				sb->s_flags |= MS_POSIXACL;
		}

		if (vol_info && vol_info->posix_paths == 0)
			cap &= ~CIFS_UNIX_POSIX_PATHNAMES_CAP;
		else if (cap & CIFS_UNIX_POSIX_PATHNAMES_CAP) {
			cFYI(1, ("negotiate posix pathnames"));
			if (sb)
				CIFS_SB(sb)->mnt_cifs_flags |=
					CIFS_MOUNT_POSIX_PATHS;
		}

		if (sb && (CIFS_SB(sb)->prepathlen > 0))
			CIFS_SB(sb)->prepath[0] = CIFS_DIR_SEP(CIFS_SB(sb));

		if (sb && (CIFS_SB(sb)->rsize > 127 * 1024)) {
			if ((cap & CIFS_UNIX_LARGE_READ_CAP) == 0) {
				CIFS_SB(sb)->rsize = 127 * 1024;
				cFYI(DBG2,
					("larger reads not supported by srv"));
			}
		}

		cFYI(1, ("Negotiate caps 0x%x", (int)cap));
#ifdef CONFIG_CIFS_DEBUG2
		if (cap & CIFS_UNIX_FCNTL_CAP)
			cFYI(1, ("FCNTL cap"));
		if (cap & CIFS_UNIX_EXTATTR_CAP)
			cFYI(1, ("EXTATTR cap"));
		if (cap & CIFS_UNIX_POSIX_PATHNAMES_CAP)
			cFYI(1, ("POSIX path cap"));
		if (cap & CIFS_UNIX_XATTR_CAP)
			cFYI(1, ("XATTR cap"));
		if (cap & CIFS_UNIX_POSIX_ACL_CAP)
			cFYI(1, ("POSIX ACL cap"));
		if (cap & CIFS_UNIX_LARGE_READ_CAP)
			cFYI(1, ("very large read cap"));
		if (cap & CIFS_UNIX_LARGE_WRITE_CAP)
			cFYI(1, ("very large write cap"));
#endif  
		if (CIFSSMBSetFSUnixInfo(xid, tcon, cap)) {
			if (vol_info == NULL) {
				cFYI(1, ("resetting capabilities failed"));
			} else
				cERROR(1, ("Negotiating Unix capabilities "
					   "with the server failed.  Consider "
					   "mounting with the Unix Extensions\n"
					   "disabled, if problems are found, "
					   "by specifying the nounix mount "
					   "option."));

		}
	}
}

static void
convert_delimiter(char *path, char delim)
{
	int i;
	char old_delim;

	if (path == NULL)
		return;

	if (delim == '/')
		old_delim = '\\';
	else
		old_delim = '/';

	for (i = 0; path[i] != '\0'; i++) {
		if (path[i] == old_delim)
			path[i] = delim;
	}
}

static void setup_cifs_sb(struct smb_vol *pvolume_info,
			  struct cifs_sb_info *cifs_sb)
{
	if (pvolume_info->rsize > CIFSMaxBufSize) {
		cERROR(1, ("rsize %d too large, using MaxBufSize",
			pvolume_info->rsize));
		cifs_sb->rsize = CIFSMaxBufSize;
	} else if ((pvolume_info->rsize) &&
			(pvolume_info->rsize <= CIFSMaxBufSize))
		cifs_sb->rsize = pvolume_info->rsize;
	else  
		cifs_sb->rsize = CIFSMaxBufSize;

	if (pvolume_info->wsize > PAGEVEC_SIZE * PAGE_CACHE_SIZE) {
		cERROR(1, ("wsize %d too large, using 4096 instead",
			  pvolume_info->wsize));
		cifs_sb->wsize = 4096;
	} else if (pvolume_info->wsize)
		cifs_sb->wsize = pvolume_info->wsize;
	else
		cifs_sb->wsize = min_t(const int,
					PAGEVEC_SIZE * PAGE_CACHE_SIZE,
					127*1024);
		 
	if (cifs_sb->rsize < 2048) {
		cifs_sb->rsize = 2048;
		 
		cFYI(1, ("readsize set to minimum: 2048"));
	}
	 
	cifs_sb->prepath = pvolume_info->prepath;
	if (cifs_sb->prepath) {
		cifs_sb->prepathlen = strlen(cifs_sb->prepath);
		 
		pvolume_info->prepath = NULL;
	} else
		cifs_sb->prepathlen = 0;
	cifs_sb->mnt_uid = pvolume_info->linux_uid;
	cifs_sb->mnt_gid = pvolume_info->linux_gid;
	cifs_sb->mnt_file_mode = pvolume_info->file_mode;
	cifs_sb->mnt_dir_mode = pvolume_info->dir_mode;
	cFYI(1, ("file mode: 0x%x  dir mode: 0x%x",
		cifs_sb->mnt_file_mode, cifs_sb->mnt_dir_mode));

	if (pvolume_info->noperm)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_NO_PERM;
	if (pvolume_info->setuids)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_SET_UID;
	if (pvolume_info->server_ino)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_SERVER_INUM;
	if (pvolume_info->remap)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_MAP_SPECIAL_CHR;
	if (pvolume_info->no_xattr)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_NO_XATTR;
	if (pvolume_info->sfu_emul)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_UNX_EMUL;
	if (pvolume_info->nobrl)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_NO_BRL;
	if (pvolume_info->nostrictsync)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_NOSSYNC;
	if (pvolume_info->mand_lock)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_NOPOSIXBRL;
	if (pvolume_info->cifs_acl)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_CIFS_ACL;
	if (pvolume_info->override_uid)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_OVERR_UID;
	if (pvolume_info->override_gid)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_OVERR_GID;
	if (pvolume_info->dynperm)
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_DYNPERM;
	if (pvolume_info->direct_io) {
		cFYI(1, ("mounting share using direct i/o"));
		cifs_sb->mnt_cifs_flags |= CIFS_MOUNT_DIRECT_IO;
	}

	if ((pvolume_info->cifs_acl) && (pvolume_info->dynperm))
		cERROR(1, ("mount option dynperm ignored if cifsacl "
			   "mount option supported"));
}

static int
is_path_accessible(int xid, struct cifsTconInfo *tcon,
		   struct cifs_sb_info *cifs_sb, const char *full_path)
{
	int rc;
	FILE_ALL_INFO *pfile_info;

	pfile_info = kmalloc(sizeof(FILE_ALL_INFO), GFP_KERNEL);
	if (pfile_info == NULL)
		return -ENOMEM;

	rc = CIFSSMBQPathInfo(xid, tcon, full_path, pfile_info,
			      0  , cifs_sb->local_nls,
			      cifs_sb->mnt_cifs_flags &
				CIFS_MOUNT_MAP_SPECIAL_CHR);
	kfree(pfile_info);
	return rc;
}

static void
cleanup_volume_info(struct smb_vol **pvolume_info)
{
	struct smb_vol *volume_info;

	if (!pvolume_info && !*pvolume_info)
		return;

	volume_info = *pvolume_info;
	kzfree(volume_info->password);
	kfree(volume_info->UNC);
	kfree(volume_info->prepath);
	kfree(volume_info);
	*pvolume_info = NULL;
	return;
}

#ifdef CONFIG_CIFS_DFS_UPCALL
 
static char *
build_unc_path_to_root(const struct smb_vol *volume_info,
		const struct cifs_sb_info *cifs_sb)
{
	char *full_path;

	int unc_len = strnlen(volume_info->UNC, MAX_TREE_SIZE + 1);
	full_path = kmalloc(unc_len + cifs_sb->prepathlen + 1, GFP_KERNEL);
	if (full_path == NULL)
		return ERR_PTR(-ENOMEM);

	strncpy(full_path, volume_info->UNC, unc_len);
	if (cifs_sb->mnt_cifs_flags & CIFS_MOUNT_POSIX_PATHS) {
		int i;
		for (i = 0; i < unc_len; i++) {
			if (full_path[i] == '\\')
				full_path[i] = '/';
		}
	}

	if (cifs_sb->prepathlen)
		strncpy(full_path + unc_len, cifs_sb->prepath,
				cifs_sb->prepathlen);

	full_path[unc_len + cifs_sb->prepathlen] = 0;  
	return full_path;
}
#endif

int
cifs_mount(struct super_block *sb, struct cifs_sb_info *cifs_sb,
		char *mount_data_global, const char *devname)
{
	int rc;
	int xid;
	struct smb_vol *volume_info;
	struct cifsSesInfo *pSesInfo;
	struct cifsTconInfo *tcon;
	struct TCP_Server_Info *srvTcp;
	char   *full_path;
	char *mount_data = mount_data_global;
#ifdef CONFIG_CIFS_DFS_UPCALL
	struct dfs_info3_param *referrals = NULL;
	unsigned int num_referrals = 0;
	int referral_walks_count = 0;
try_mount_again:
#endif
	rc = 0;
	tcon = NULL;
	pSesInfo = NULL;
	srvTcp = NULL;
	full_path = NULL;

	xid = GetXid();

	volume_info = kzalloc(sizeof(struct smb_vol), GFP_KERNEL);
	if (!volume_info) {
		rc = -ENOMEM;
		goto out;
	}

	if (cifs_parse_mount_options(mount_data, devname, volume_info)) {
		rc = -EINVAL;
		goto out;
	}

	if (volume_info->nullauth) {
		cFYI(1, ("null user"));
		volume_info->username = "";
	} else if (volume_info->username) {
		 
		cFYI(1, ("Username: %s", volume_info->username));
	} else {
		cifserror("No username specified");
	 
		rc = -EINVAL;
		goto out;
	}

	if (volume_info->iocharset == NULL) {
		cifs_sb->local_nls = load_nls_default();
	 
	} else {
		cifs_sb->local_nls = load_nls(volume_info->iocharset);
		if (cifs_sb->local_nls == NULL) {
			cERROR(1, ("CIFS mount error: iocharset %s not found",
				 volume_info->iocharset));
			rc = -ELIBACC;
			goto out;
		}
	}

	srvTcp = cifs_get_tcp_session(volume_info);
	if (IS_ERR(srvTcp)) {
		rc = PTR_ERR(srvTcp);
		goto out;
	}

	pSesInfo = cifs_find_smb_ses(srvTcp, volume_info->username);
	if (pSesInfo) {
		cFYI(1, ("Existing smb sess found (status=%d)",
			pSesInfo->status));
		 
		cifs_put_tcp_session(srvTcp);

		down(&pSesInfo->sesSem);
		if (pSesInfo->need_reconnect) {
			cFYI(1, ("Session needs reconnect"));
			rc = cifs_setup_session(xid, pSesInfo,
						cifs_sb->local_nls);
		}
		up(&pSesInfo->sesSem);
	} else if (!rc) {
		cFYI(1, ("Existing smb sess not found"));
		pSesInfo = sesInfoAlloc();
		if (pSesInfo == NULL) {
			rc = -ENOMEM;
			goto mount_fail_check;
		}

		pSesInfo->server = srvTcp;
		if (srvTcp->addr.sockAddr6.sin6_family == AF_INET6)
			sprintf(pSesInfo->serverName, "%pI6",
				&srvTcp->addr.sockAddr6.sin6_addr);
		else
			sprintf(pSesInfo->serverName, "%pI4",
				&srvTcp->addr.sockAddr.sin_addr.s_addr);

		write_lock(&cifs_tcp_ses_lock);
		list_add(&pSesInfo->smb_ses_list, &srvTcp->smb_ses_list);
		write_unlock(&cifs_tcp_ses_lock);

		if (volume_info->password) {
			pSesInfo->password = kstrdup(volume_info->password,
						     GFP_KERNEL);
			if (!pSesInfo->password) {
				rc = -ENOMEM;
				goto mount_fail_check;
			}
		}
		if (volume_info->username)
			strncpy(pSesInfo->userName, volume_info->username,
				MAX_USERNAME_SIZE);
		if (volume_info->domainname) {
			int len = strlen(volume_info->domainname);
			pSesInfo->domainName = kmalloc(len + 1, GFP_KERNEL);
			if (pSesInfo->domainName)
				strcpy(pSesInfo->domainName,
					volume_info->domainname);
		}
		pSesInfo->linux_uid = volume_info->linux_uid;
		pSesInfo->overrideSecFlg = volume_info->secFlg;
		down(&pSesInfo->sesSem);

		rc = cifs_setup_session(xid, pSesInfo,
					cifs_sb->local_nls);
		up(&pSesInfo->sesSem);
	}

	if (!rc) {
		setup_cifs_sb(volume_info, cifs_sb);

		tcon = cifs_find_tcon(pSesInfo, volume_info->UNC);
		if (tcon) {
			cFYI(1, ("Found match on UNC path"));
			 
			cifs_put_smb_ses(pSesInfo);
			if (tcon->seal != volume_info->seal)
				cERROR(1, ("transport encryption setting "
					   "conflicts with existing tid"));
		} else {
			tcon = tconInfoAlloc();
			if (tcon == NULL) {
				rc = -ENOMEM;
				goto mount_fail_check;
			}

			tcon->ses = pSesInfo;
			if (volume_info->password) {
				tcon->password = kstrdup(volume_info->password,
							 GFP_KERNEL);
				if (!tcon->password) {
					rc = -ENOMEM;
					goto mount_fail_check;
				}
			}

			if ((strchr(volume_info->UNC + 3, '\\') == NULL)
			    && (strchr(volume_info->UNC + 3, '/') == NULL)) {
				cERROR(1, ("Missing share name"));
				rc = -ENODEV;
				goto mount_fail_check;
			} else {
				 
				rc = CIFSTCon(xid, pSesInfo, volume_info->UNC,
					      tcon, cifs_sb->local_nls);
				cFYI(1, ("CIFS Tcon rc = %d", rc));
				if (volume_info->nodfs) {
					tcon->Flags &= ~SMB_SHARE_IS_IN_DFS;
					cFYI(1, ("DFS disabled (%d)",
						tcon->Flags));
				}
			}
			if (rc)
				goto remote_path_check;
			tcon->seal = volume_info->seal;
			write_lock(&cifs_tcp_ses_lock);
			list_add(&tcon->tcon_list, &pSesInfo->tcon_list);
			write_unlock(&cifs_tcp_ses_lock);
		}

		tcon->retry = volume_info->retry;
		tcon->nocase = volume_info->nocase;
		tcon->local_lease = volume_info->local_lease;
	}
	if (pSesInfo) {
		if (pSesInfo->capabilities & CAP_LARGE_FILES)
			sb->s_maxbytes = MAX_LFS_FILESIZE;
		else
			sb->s_maxbytes = MAX_NON_LFS;
	}

	sb->s_time_gran = 100;

	if (rc)
		goto remote_path_check;

	cifs_sb->tcon = tcon;

	if (!tcon->ipc) {
		CIFSSMBQFSDeviceInfo(xid, tcon);
		CIFSSMBQFSAttributeInfo(xid, tcon);
	}

	if (tcon->ses->capabilities & CAP_UNIX)
		 
		reset_cifs_unix_caps(xid, tcon, sb, volume_info);
	else
		tcon->unix_ext = 0;  

	if ((cifs_sb->mnt_cifs_flags & CIFS_MOUNT_POSIX_PATHS) == 0)
		convert_delimiter(cifs_sb->prepath, CIFS_DIR_SEP(cifs_sb));

	if ((tcon->unix_ext == 0) && (cifs_sb->rsize > (1024 * 127))) {
		cifs_sb->rsize = 1024 * 127;
		cFYI(DBG2, ("no very large read support, rsize now 127K"));
	}
	if (!(tcon->ses->capabilities & CAP_LARGE_WRITE_X))
		cifs_sb->wsize = min(cifs_sb->wsize,
			       (tcon->ses->server->maxBuf - MAX_CIFS_HDR_SIZE));
	if (!(tcon->ses->capabilities & CAP_LARGE_READ_X))
		cifs_sb->rsize = min(cifs_sb->rsize,
			       (tcon->ses->server->maxBuf - MAX_CIFS_HDR_SIZE));

remote_path_check:
	 
	if (!rc && cifs_sb->prepathlen && tcon) {
		 
		full_path = cifs_build_path_to_root(cifs_sb);
		if (full_path == NULL) {
			rc = -ENOMEM;
			goto mount_fail_check;
		}
		rc = is_path_accessible(xid, tcon, cifs_sb, full_path);
		if (rc != -EREMOTE) {
			kfree(full_path);
			goto mount_fail_check;
		}
		kfree(full_path);
	}

	if (rc == -EREMOTE) {
#ifdef CONFIG_CIFS_DFS_UPCALL
		if (referral_walks_count > MAX_NESTED_LINKS) {
			 
			rc = -ELOOP;
			goto mount_fail_check;
		}
		 
		if ((cifs_sb->mnt_cifs_flags & CIFS_MOUNT_POSIX_PATHS) == 0)
			convert_delimiter(cifs_sb->prepath,
					CIFS_DIR_SEP(cifs_sb));
		full_path = build_unc_path_to_root(volume_info, cifs_sb);
		if (IS_ERR(full_path)) {
			rc = PTR_ERR(full_path);
			goto mount_fail_check;
		}

		cFYI(1, ("Getting referral for: %s", full_path));
		rc = get_dfs_path(xid, pSesInfo , full_path + 1,
			cifs_sb->local_nls, &num_referrals, &referrals,
			cifs_sb->mnt_cifs_flags & CIFS_MOUNT_MAP_SPECIAL_CHR);
		if (!rc && num_referrals > 0) {
			char *fake_devname = NULL;

			if (mount_data != mount_data_global)
				kfree(mount_data);

			mount_data = cifs_compose_mount_options(
					cifs_sb->mountdata, full_path + 1,
					referrals, &fake_devname);

			free_dfs_info_array(referrals, num_referrals);
			kfree(fake_devname);
			kfree(full_path);

			if (IS_ERR(mount_data)) {
				rc = PTR_ERR(mount_data);
				mount_data = NULL;
				goto mount_fail_check;
			}

			if (tcon)
				cifs_put_tcon(tcon);
			else if (pSesInfo)
				cifs_put_smb_ses(pSesInfo);

			cleanup_volume_info(&volume_info);
			referral_walks_count++;
			FreeXid(xid);
			goto try_mount_again;
		}
#else  
		rc = -EOPNOTSUPP;
#endif
	}

mount_fail_check:
	 
	if (rc) {
		if (mount_data != mount_data_global)
			kfree(mount_data);
		 
		if (tcon)
			cifs_put_tcon(tcon);
		else if (pSesInfo)
			cifs_put_smb_ses(pSesInfo);
		else
			cifs_put_tcp_session(srvTcp);
		goto out;
	}

out:
	 
	cleanup_volume_info(&volume_info);
	FreeXid(xid);
	return rc;
}

int
CIFSTCon(unsigned int xid, struct cifsSesInfo *ses,
	 const char *tree, struct cifsTconInfo *tcon,
	 const struct nls_table *nls_codepage)
{
	struct smb_hdr *smb_buffer;
	struct smb_hdr *smb_buffer_response;
	TCONX_REQ *pSMB;
	TCONX_RSP *pSMBr;
	unsigned char *bcc_ptr;
	int rc = 0;
	int length, bytes_left;
	__u16 count;

	if (ses == NULL)
		return -EIO;

	smb_buffer = cifs_buf_get();
	if (smb_buffer == NULL)
		return -ENOMEM;

	smb_buffer_response = smb_buffer;

	header_assemble(smb_buffer, SMB_COM_TREE_CONNECT_ANDX,
			NULL   , 4   );

	smb_buffer->Mid = GetNextMid(ses->server);
	smb_buffer->Uid = ses->Suid;
	pSMB = (TCONX_REQ *) smb_buffer;
	pSMBr = (TCONX_RSP *) smb_buffer_response;

	pSMB->AndXCommand = 0xFF;
	pSMB->Flags = cpu_to_le16(TCON_EXTENDED_SECINFO);
	bcc_ptr = &pSMB->Password[0];
	if ((ses->server->secMode) & SECMODE_USER) {
		pSMB->PasswordLength = cpu_to_le16(1);	 
		*bcc_ptr = 0;  
		bcc_ptr++;               
		 
	} else {
		pSMB->PasswordLength = cpu_to_le16(CIFS_SESS_KEY_SIZE);
		 
#ifdef CONFIG_CIFS_WEAK_PW_HASH
		if ((extended_security & CIFSSEC_MAY_LANMAN) &&
		    (ses->server->secType == LANMAN))
			calc_lanman_hash(tcon->password, ses->server->cryptKey,
					 ses->server->secMode &
					    SECMODE_PW_ENCRYPT ? true : false,
					 bcc_ptr);
		else
#endif  
		SMBNTencrypt(tcon->password, ses->server->cryptKey,
			     bcc_ptr);

		bcc_ptr += CIFS_SESS_KEY_SIZE;
		if (ses->capabilities & CAP_UNICODE) {
			 
			*bcc_ptr = 0;  
			bcc_ptr++;
		}
	}

	if (ses->server->secMode &
			(SECMODE_SIGN_REQUIRED | SECMODE_SIGN_ENABLED))
		smb_buffer->Flags2 |= SMBFLG2_SECURITY_SIGNATURE;

	if (ses->capabilities & CAP_STATUS32) {
		smb_buffer->Flags2 |= SMBFLG2_ERR_STATUS;
	}
	if (ses->capabilities & CAP_DFS) {
		smb_buffer->Flags2 |= SMBFLG2_DFS;
	}
	if (ses->capabilities & CAP_UNICODE) {
		smb_buffer->Flags2 |= SMBFLG2_UNICODE;
		length =
		    cifs_strtoUCS((__le16 *) bcc_ptr, tree,
			6   *
			(  + 256  ), nls_codepage);
		bcc_ptr += 2 * length;	 
		bcc_ptr += 2;	 
	} else {		 
		strcpy(bcc_ptr, tree);
		bcc_ptr += strlen(tree) + 1;
	}
	strcpy(bcc_ptr, "?????");
	bcc_ptr += strlen("?????");
	bcc_ptr += 1;
	count = bcc_ptr - &pSMB->Password[0];
	pSMB->hdr.smb_buf_length += count;
	pSMB->ByteCount = cpu_to_le16(count);

	rc = SendReceive(xid, ses, smb_buffer, smb_buffer_response, &length,
			 CIFS_STD_OP);

	if ((rc == 0) && (tcon != NULL)) {
		bool is_unicode;

		tcon->tidStatus = CifsGood;
		tcon->need_reconnect = false;
		tcon->tid = smb_buffer_response->Tid;
		bcc_ptr = pByteArea(smb_buffer_response);
		bytes_left = BCC(smb_buffer_response);
		length = strnlen(bcc_ptr, bytes_left - 2);
		if (smb_buffer->Flags2 & SMBFLG2_UNICODE)
			is_unicode = true;
		else
			is_unicode = false;

		if (length == 3) {
			if ((bcc_ptr[0] == 'I') && (bcc_ptr[1] == 'P') &&
			    (bcc_ptr[2] == 'C')) {
				cFYI(1, ("IPC connection"));
				tcon->ipc = 1;
			}
		} else if (length == 2) {
			if ((bcc_ptr[0] == 'A') && (bcc_ptr[1] == ':')) {
				 
				cFYI(1, ("disk share connection"));
			}
		}
		bcc_ptr += length + 1;
		bytes_left -= (length + 1);
		strncpy(tcon->treeName, tree, MAX_TREE_SIZE);

		kfree(tcon->nativeFileSystem);
		tcon->nativeFileSystem = cifs_strndup_from_ucs(bcc_ptr,
						      bytes_left, is_unicode,
						      nls_codepage);

		cFYI(1, ("nativeFileSystem=%s", tcon->nativeFileSystem));

		if ((smb_buffer_response->WordCount == 3) ||
			 (smb_buffer_response->WordCount == 7))
			 
			tcon->Flags = le16_to_cpu(pSMBr->OptionalSupport);
		else
			tcon->Flags = 0;
		cFYI(1, ("Tcon flags: 0x%x ", tcon->Flags));
	} else if ((rc == 0) && tcon == NULL) {
		 
		ses->ipc_tid = smb_buffer_response->Tid;
	}

	cifs_buf_release(smb_buffer);
	return rc;
}

int
cifs_umount(struct super_block *sb, struct cifs_sb_info *cifs_sb)
{
	int rc = 0;
	char *tmp;

	if (cifs_sb->tcon)
		cifs_put_tcon(cifs_sb->tcon);

	cifs_sb->tcon = NULL;
	tmp = cifs_sb->prepath;
	cifs_sb->prepathlen = 0;
	cifs_sb->prepath = NULL;
	kfree(tmp);

	return rc;
}

int cifs_setup_session(unsigned int xid, struct cifsSesInfo *pSesInfo,
					   struct nls_table *nls_info)
{
	int rc = 0;
	int first_time = 0;
	struct TCP_Server_Info *server = pSesInfo->server;

	if (server->maxBuf == 0)   {
		rc = CIFSSMBNegotiate(xid, pSesInfo);
		if (rc == -EAGAIN) {
			 
			rc = CIFSSMBNegotiate(xid, pSesInfo);
			if (rc == -EAGAIN)
				rc = -EHOSTDOWN;
		}
		if (rc == 0) {
			spin_lock(&GlobalMid_Lock);
			if (server->tcpStatus != CifsExiting)
				server->tcpStatus = CifsGood;
			else
				rc = -EHOSTDOWN;
			spin_unlock(&GlobalMid_Lock);

		}
		first_time = 1;
	}

	if (rc)
		goto ss_err_exit;

	pSesInfo->flags = 0;
	pSesInfo->capabilities = server->capabilities;
	if (linuxExtEnabled == 0)
		pSesInfo->capabilities &= (~CAP_UNIX);

	cFYI(1, ("Security Mode: 0x%x Capabilities: 0x%x TimeAdjust: %d",
		 server->secMode, server->capabilities, server->timeAdj));

	rc = CIFS_SessSetup(xid, pSesInfo, first_time, nls_info);
	if (rc) {
		cERROR(1, ("Send error in SessSetup = %d", rc));
	} else {
		cFYI(1, ("CIFS Session Established successfully"));
		spin_lock(&GlobalMid_Lock);
		pSesInfo->status = CifsGood;
		pSesInfo->need_reconnect = false;
		spin_unlock(&GlobalMid_Lock);
	}

ss_err_exit:
	return rc;
}
