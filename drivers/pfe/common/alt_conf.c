/*
 *  Copyright (c) 2009 Mindspeed Technologies, Inc.
 *
 *  THIS FILE IS CONFIDENTIAL.  
 *  
 *  AUTHORIZED USE IS GOVERNED BY CONFIDENTIALITY AND LICENSE AGREEMENTS WITH MINDSPEED TECHNOLOGIES, INC.
 *
 *  UNAUTHORIZED COPIES AND USE ARE STRICTLY PROHIBITED AND MAY RESULT IN CRIMINAL AND/OR CIVIL PROSECUTION.
 */


#include "modules.h"
#include "fpp.h"
#include "fe.h"
#include "alt_conf.h"
#include "module_mc4.h"
#include "module_mc6.h"


#if !defined(COMCERTO_2000)
void update_mc_ttl_check(void)
{

}
#else
void update_mc_ttl_check(void)
{
	int id;

	for (id=CLASS0_ID; id <= CLASS_MAX_ID; id++) {
		pe_dmem_write(id, gMC4Ctx.ttl_check_rule, virt_to_class_dmem(&gMC4Ctx.ttl_check_rule), 4);
		pe_dmem_write(id, gMC6Ctx.hoplimit_check_rule, virt_to_class_dmem(&gMC6Ctx.hoplimit_check_rule), 4);
	}
}
#endif

int ALTCONF_HandleCONF_SET (U16 *p, U16 Length)
{
	AltConfCommandSet cmd[ALTCONF_OPTION_MAX];
	int rc = NO_ERR;
	int i = 0;
	U16 cmd_len = 0;

	// Check length
	if (Length > (sizeof(cmd)*ALTCONF_OPTION_MAX))
		return ERR_WRONG_COMMAND_SIZE;

	// Ensure alignment
	SFL_memcpy((U8*)&cmd, (U8*)p, sizeof(cmd));

	while(cmd_len < Length)
	{
		switch(cmd[i].option_id)
		{
			case ALTCONF_OPTION_MCTTL:
				// Check configuration value range
				if(cmd[i].num_params != ALTCONF_MCTTL_NUM_PARAMS)
					return ERR_ALTCONF_WRONG_NUM_PARAMS;

				if(cmd[i].params[0] > ALTCONF_MCTTL_MODE_MAX)
					return ERR_ALTCONF_MODE_NOT_SUPPORTED;

				// update processed command length
				cmd_len += (ALTCONF_MCTTL_NUM_PARAMS*sizeof(U32) + 2*sizeof(U16));

				// Update MC4 MC6 context with TTL check rule
				gMC4Ctx.ttl_check_rule = (U8)cmd[i].params[0];
				gMC6Ctx.hoplimit_check_rule = (U8)cmd[i].params[0];
				update_mc_ttl_check();
				break;

#ifdef COMCERTO_1000
                     /* This alternate configuration currently supported on C1K only */
			case ALTCONF_OPTION_IPSECRL:
				// Check number of parameters. Can be 3 or 2.
				if(cmd[i].num_params != ALTCONF_IPSECRL_NUM_PARAMS) 
					return ERR_ALTCONF_WRONG_NUM_PARAMS;

				if(cmd[i].params[0]  == ALTCONF_IPSECRL_ON)
				{
					AltConfIpsec_RateLimitEntry.enable = TRUE;
					AltConfIpsec_RateLimitEntry.aggregate_bandwidth = cmd[i].params[1];
					/* Tokens stored in bits per 1ms clock period = (rate * bps_PER_Kbps)/MILLISEC_PER_SEC
					  * bps_PER_Kbps = 1000 & MILLISEC_PER_SEC = 1000
					  * Hence tokens per 1ms clock period = rate */
	       			AltConfIpsec_RateLimitEntry.tokens_per_clock_period = (cmd[i].params[1]);
					//Initially No Tokens are available. They will be accrued as clock ticks i.e every 1ms. 
					AltConfIpsec_RateLimitEntry.inbound_tokens_available = 0;
					AltConfIpsec_RateLimitEntry.outbound_tokens_available = 0;

					AltConfIpsec_RateLimitEntry.bucket_size = cmd[i].params[2];
					/* If bucket size is less than 4*tokens_per_clock_period, change bucket_size */
					if(cmd[i].params[2] < AltConfIpsec_RateLimitEntry.tokens_per_clock_period * 4)
					{
						AltConfIpsec_RateLimitEntry.bucket_size = AltConfIpsec_RateLimitEntry.tokens_per_clock_period * 4;
					}

					if(AltConfIpsec_RateLimitEntry.bucket_size < ALTCONF_RX_MTU)
						AltConfIpsec_RateLimitEntry.bucket_size = ALTCONF_RX_MTU;

					/* register IPSEC module to the timer service for updating tokens every 1ms */
					timer_add(&ipsec_ratelimiter_timer, 1);
				}
 				else if(cmd[i].params[0] == ALTCONF_IPSECRL_OFF)
 				{
					AltConfIpsec_RateLimitEntry.enable = FALSE;
					timer_del(&ipsec_ratelimiter_timer);

 				}
				else
					return ERR_ALTCONF_MODE_NOT_SUPPORTED;

				// update processed command length
				cmd_len += (ALTCONF_IPSECRL_NUM_PARAMS*sizeof(U32) + 2*sizeof(U16));

				break;		
#endif /* #ifdef COMCERTO_1000 */
	
			default:
				return ERR_ALTCONF_OPTION_NOT_SUPPORTED;
		}
	}

	return rc;
}


int ALTCONF_HandleCONF_RESET_ALL (U16 *p, U16 Length)
{
	int rc = NO_ERR;
		
	// Update MC4 MC6 context with TTL check rule
	gMC4Ctx.ttl_check_rule = ALTCONF_MCTTL_MODE_DEFAULT;
	gMC6Ctx.hoplimit_check_rule = ALTCONF_MCTTL_MODE_DEFAULT;
	update_mc_ttl_check();

#ifdef COMCERTO_1000
	AltConfIpsec_RateLimitEntry.enable = FALSE;
	timer_del(&ipsec_ratelimiter_timer);
#endif /* #ifdef COMCERTO_1000 */

	return rc;

}

#ifdef COMCERTO_1000
/*****************************************************************************
*Function Name : AltConfIpsec_RateLimit_token_generator
*Description: This is 1ms timer function which replenishes the tokens. For every 1 ms, it 
*                   increments tokens_available variable by the value of pre-computed 
*                   tokens_per_clock_period. If tokens_available value is greater than bucket_size, 
*                   the value of tokens_available is set to bucket_size. 
******************************************************************************/
void AltConfIpsec_RateLimit_token_generator(void)
{

	// Updating the tokens_available value of Rate Limiting node 

	// Incrementing by the value of tokens_per_clock_period
	AltConfIpsec_RateLimitEntry.inbound_tokens_available += AltConfIpsec_RateLimitEntry.tokens_per_clock_period;
	AltConfIpsec_RateLimitEntry.outbound_tokens_available += AltConfIpsec_RateLimitEntry.tokens_per_clock_period;

	// Make sure that tokens_available are not greater than the bucket size
	if(AltConfIpsec_RateLimitEntry.inbound_tokens_available > AltConfIpsec_RateLimitEntry.bucket_size)
		AltConfIpsec_RateLimitEntry.inbound_tokens_available = AltConfIpsec_RateLimitEntry.bucket_size;		
	
	if(AltConfIpsec_RateLimitEntry.outbound_tokens_available > AltConfIpsec_RateLimitEntry.bucket_size)
		AltConfIpsec_RateLimitEntry.outbound_tokens_available = AltConfIpsec_RateLimitEntry.bucket_size;		
	
}


/*****************************************************************************
*Function Name : AltConfIpsec_Rate_Limiter
*Description : This is the IPSEC Rate limiting function. A check is made if the packet  
*                    can be allowed for inbound or outbound for the configured bandwidth. 
*                    If yes, check is made if the size of the packet is 
*                    less than the tokens_available. If it is not, FALSE is returned. If it is less, 
*                    tokens_available is decremented by the size of the packet. TRUE is returned.
*                    mode: inbound/outbound
******************************************************************************/
U8 AltConfIpsec_Rate_Limiter(int mode, U32 sizeofpacket) 
{

      /* This function is called only when rate limiting is enabled.
        * Caller will check if IPSEC Rate Limiting is enabled or not */
		
	// Check that sizeofpacket is less than or equal to tokens_available. Also taking into account Ethernet FCS size
	
	if(mode == ALTCONF_IPSEC_INBOUND)
	{
		if(AltConfIpsec_RateLimitEntry.inbound_tokens_available > 0)
 		{
			AltConfIpsec_RateLimitEntry.inbound_tokens_available -= (sizeofpacket*ALTCONF_BITS_PER_BYTE);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	else if(mode == ALTCONF_IPSEC_OUTBOUND)
	{
		if(AltConfIpsec_RateLimitEntry.outbound_tokens_available > 0)
 		{
			AltConfIpsec_RateLimitEntry.outbound_tokens_available -= (sizeofpacket*ALTCONF_BITS_PER_BYTE);
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}

	return TRUE;
      
			 
 	
}
#endif /* #ifdef COMCERTO_1000 */

