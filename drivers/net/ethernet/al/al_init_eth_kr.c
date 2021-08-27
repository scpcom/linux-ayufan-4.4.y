 
#include "al_init_eth_kr.h"
#include "al_hal_serdes.h"

#define AL_ETH_KR_AN_TIMEOUT		(500)
#ifdef CONFIG_SYNO_ALPINE_A0
#define AL_ETH_KR_EYE_MEASURE_TIMEOUT	(100)
#endif
 
#define AL_ETH_KR_FRAME_LOCK_TIMEOUT	(500 * 1000)
#ifdef CONFIG_SYNO_ALPINE_A0
 
#else
#define AL_ETH_KR_EYE_MEASURE_TIMEOUT	(32 * 1000)
#endif
#define AL_ETH_KR_LT_DONE_TIMEOUT	(500 * 1000)
 
#ifdef CONFIG_SYNO_ALPINE_A0
#define AL_ETH_KR_LT_MAX_ROUNDS		(50000)
#else
#define AL_ETH_KR_LT_MAX_ROUNDS		(15000)
#endif

enum al_eth_kr_mac_lt_state {
	TX_INIT = 0,	 
	WAIT_BEGIN,	 
	DO_PRESET,	 
	DO_HOLD,	 
	 
	QMEASURE,	 
	QCHECK,		 
	DO_NEXT_TRY,	 
	END_STEPS,	 
	END_STEPS_HOLD,	 
	COEFF_DONE,	 
	 
	SET_READY,	 
	TX_DONE		 
};

static char *al_eth_kr_mac_sm_name[] = {"TX_INIT", "WAIT_BEGIN", "DO_PRESET",
					"DO_HOLD", "QMEASURE", "QCHECK",
					"DO_NEXT_TRY", "END_STEPS",
					"END_STEPS_HOLD", "COEFF_DONE",
					"SET_READY","TX_DONE"};

enum al_eth_kr_coef {
	AL_ETH_KR_COEF_C_MINUS,
	AL_ETH_KR_COEF_C_ZERO,
	AL_ETH_KR_COEF_C_PLUS,
};

#define COEFF_TO_MANIPULATE AL_ETH_KR_COEF_C_MINUS
#ifdef CONFIG_SYNO_ALPINE_A0
#define COEFF_TO_MANIPULATE_LAST AL_ETH_KR_COEF_C_MINUS
#else
#define COEFF_TO_MANIPULATE_LAST AL_ETH_KR_COEF_C_PLUS
#endif
#define QARRAY_SIZE	3  

struct al_eth_kr_data {
	struct al_hal_eth_adapter	*adapter;
	struct al_serdes_obj		*serdes_obj;
	enum al_serdes_group		grp;
	enum al_serdes_lane		lane;

	struct al_eth_kr_status_report_data status_report;  
	struct al_eth_kr_coef_up_data last_lpcoeff;  

	enum al_eth_kr_mac_lt_state algo_state;	 
	unsigned int qarray[QARRAY_SIZE];	 
	unsigned int qarray_cnt;         
	enum al_eth_kr_coef curr_coeff;
	unsigned int coeff_status_step;  
	unsigned int end_steps_cnt;      
};

static int al_eth_kr_an_run(struct al_eth_kr_data *kr_data,
			struct al_eth_an_adv *an_adv,
			struct al_eth_an_adv *an_partner_adv)
{
	int rc;
	al_bool page_received = AL_FALSE;
	al_bool an_completed = AL_FALSE;
	al_bool error = AL_FALSE;
	int timeout = AL_ETH_KR_AN_TIMEOUT;

	rc = al_eth_kr_an_init(kr_data->adapter, an_adv);
	if (rc) {
		al_err("%s %s autonegotiation init failed\n",
			kr_data->adapter->name, __func__);
		return rc;
	}

	rc = al_eth_kr_an_start(kr_data->adapter, AL_TRUE);
	if (rc) {
		al_err("%s %s autonegotiation enable failed\n",
			kr_data->adapter->name,	__func__);
		return rc;
	}

	do {
		al_msleep(10);
		timeout -= 10;
		if (timeout <= 0) {
			al_info("%s %s autonegotiation failed on timeout\n",
				kr_data->adapter->name, __func__);

			return -ETIMEDOUT;
		}

		al_eth_kr_an_status_check(kr_data->adapter, &page_received,
					  &an_completed, &error);
	} while (page_received == AL_FALSE);

	if (error != 0) {
		al_info("%s %s autonegotiation failed (status error)\n",
				kr_data->adapter->name, __func__);

			return -EIO;
	}

	al_eth_kr_an_read_adv(kr_data->adapter, an_partner_adv);

	al_dbg("%s %s autonegotiation completed. error = %d\n",
			kr_data->adapter->name,	__func__, error);

	return 0;
}

static enum al_eth_kr_cl72_cstate al_eth_lt_coeff_set(
			struct al_eth_kr_data *kr_data,
			enum al_serdes_tx_deemph_param param,
			uint32_t op)
{
	enum al_eth_kr_cl72_cstate status = 0;

	switch(op) {
		case AL_PHY_KR_COEF_UP_HOLD:
			 
			status = C72_CSTATE_NOT_UPDATED;
			break;
		case AL_PHY_KR_COEF_UP_INC:
			status = C72_CSTATE_UPDATED;

			if(!al_serdes_tx_deemph_inc(kr_data->serdes_obj,
						    kr_data->grp,
						    kr_data->lane,
						    param))
				status = C72_CSTATE_MAX;

			break;
		case AL_PHY_KR_COEF_UP_DEC:
			status = C72_CSTATE_UPDATED;

			if(!al_serdes_tx_deemph_dec(kr_data->serdes_obj,
						    kr_data->grp,
						    kr_data->lane,
						    param))
				status = C72_CSTATE_MIN;

			break;
		default:  
			break;
	}

	return status;
}

void al_eth_coeff_req_handle(struct al_eth_kr_data *kr_data,
			     struct al_eth_kr_coef_up_data *lpcoeff)
{
	struct al_eth_kr_status_report_data *report = &kr_data->status_report;

	if (lpcoeff->preset || lpcoeff->initialize) {
		al_serdes_tx_deemph_preset(kr_data->serdes_obj,
					   kr_data->grp,
					   kr_data->lane);

		report->c_minus = C72_CSTATE_UPDATED;

		report->c_plus = C72_CSTATE_UPDATED;

		report->c_zero = C72_CSTATE_MAX;

		return;
	}

	report->c_minus = al_eth_lt_coeff_set(kr_data,
					      AL_SERDES_TX_DEEMP_C_MINUS,
					      lpcoeff->c_minus);

	report->c_zero = al_eth_lt_coeff_set(kr_data,
					     AL_SERDES_TX_DEEMP_C_ZERO,
					     lpcoeff->c_zero);

	report->c_plus = al_eth_lt_coeff_set(kr_data,
					     AL_SERDES_TX_DEEMP_C_PLUS,
					     lpcoeff->c_plus);

	al_dbg("%s: c(0) = 0x%x c(-1) = 0x%x c(1) = 0x%x\n",
		__func__, report->c_zero, report->c_plus, report->c_minus);
}

void al_eth_kr_lt_receiver_task_init(struct al_eth_kr_data *kr_data)
{
	al_memset(&kr_data->last_lpcoeff, 0, sizeof(struct al_eth_kr_coef_up_data));
	al_memset(&kr_data->status_report, 0, sizeof(struct al_eth_kr_status_report_data));
}

static al_bool al_eth_lp_coeff_up_change(struct al_eth_kr_data *kr_data,
					 struct al_eth_kr_coef_up_data *lpcoeff)
{
	struct al_eth_kr_coef_up_data *last_lpcoeff = &kr_data->last_lpcoeff;

	if (!al_memcmp(last_lpcoeff, lpcoeff, sizeof(struct al_eth_kr_coef_up_data)))
		return AL_FALSE;

	al_memcpy(last_lpcoeff, lpcoeff, sizeof(struct al_eth_kr_coef_up_data));
	return AL_TRUE;

}

int al_eth_kr_lt_receiver_task_run(struct al_eth_kr_data *kr_data)
{
	struct al_eth_kr_coef_up_data new_lpcoeff;

	if (!al_eth_kr_receiver_frame_lock_get(kr_data->adapter))
			return 0;

	al_eth_lp_coeff_up_get(kr_data->adapter, &new_lpcoeff);

	if (al_eth_lp_coeff_up_change(kr_data, &new_lpcoeff))
		 
		al_eth_coeff_req_handle(kr_data, &new_lpcoeff);

	return 0;
}

int al_eth_kr_lt_transmitter_task_init(struct al_eth_kr_data *kr_data)
{
	int i;
	int rc;
	uint32_t temp_val;

	for (i = 0 ; i < QARRAY_SIZE ; i++)
			kr_data->qarray[i] = 0;

	kr_data->qarray_cnt = 0;
	kr_data->algo_state = TX_INIT;
	kr_data->curr_coeff = COEFF_TO_MANIPULATE;   
	kr_data->coeff_status_step  = C72_CSTATE_NOT_UPDATED;
	kr_data->end_steps_cnt = QARRAY_SIZE-1;   

	rc = al_serdes_eye_measure_run(kr_data->serdes_obj,
				       kr_data->grp,
				       kr_data->lane,
				       AL_ETH_KR_EYE_MEASURE_TIMEOUT,
				       &temp_val);
	if (rc != 0) {
		al_warn("%s: Failed to run Rx equalizer (rc = 0x%x)\n",
			__func__, rc);

		return rc;
	}

	return 0;
}

static al_bool al_eth_kr_lt_all_not_updated(
			struct al_eth_kr_status_report_data *report)
{
	if ((report->c_zero == C72_CSTATE_NOT_UPDATED) &&
	    (report->c_minus == C72_CSTATE_NOT_UPDATED) &&
	    (report->c_plus == C72_CSTATE_NOT_UPDATED)) {
		return AL_TRUE;
	}

	return AL_FALSE;
}

static void al_eth_kr_lt_coef_set(
			struct al_eth_kr_coef_up_data *ldcoeff,
			enum al_eth_kr_coef coef,
			enum al_eth_kr_cl72_coef_op op)
{
	switch(coef) {
	case AL_ETH_KR_COEF_C_MINUS:
		ldcoeff->c_minus = op;
		break;
	case AL_ETH_KR_COEF_C_PLUS:
		ldcoeff->c_plus = op;
		break;
	case AL_ETH_KR_COEF_C_ZERO:
		ldcoeff->c_zero = op;
		break;
	}
}

static enum al_eth_kr_cl72_coef_op al_eth_kr_lt_coef_report_get(
			struct al_eth_kr_status_report_data *report,
			enum al_eth_kr_coef coef)
{
	switch(coef) {
	case AL_ETH_KR_COEF_C_MINUS:
		return report->c_minus;
	case AL_ETH_KR_COEF_C_PLUS:
		return report->c_plus;
	case AL_ETH_KR_COEF_C_ZERO:
		return report->c_zero;
	}

	return 0;
}

int al_eth_kr_lt_transmitter_task_run(struct al_eth_kr_data *kr_data)
{
	struct al_eth_kr_status_report_data report;
	unsigned int coeff_status_cur;
	struct al_eth_kr_coef_up_data ldcoeff = {0};
	unsigned int val;
	int i;
	enum al_eth_kr_mac_lt_state nextstate;
	int rc = 0;

	if (!(al_eth_kr_receiver_frame_lock_get(kr_data->adapter)))
		return 0;

	al_eth_lp_status_report_get(kr_data->adapter, &report);

	coeff_status_cur = al_eth_kr_lt_coef_report_get(&report, kr_data->curr_coeff);

	nextstate = kr_data->algo_state;  

	switch(kr_data->algo_state) {
	case TX_INIT:
		 
		if (al_eth_kr_startup_proto_prog_get(kr_data->adapter))
			 
			nextstate = WAIT_BEGIN;
		break;

	case WAIT_BEGIN:
		kr_data->qarray_cnt          = 0;
		kr_data->curr_coeff           = COEFF_TO_MANIPULATE;
		kr_data->coeff_status_step   = C72_CSTATE_NOT_UPDATED;
		coeff_status_cur    = C72_CSTATE_NOT_UPDATED;
		kr_data->end_steps_cnt       = QARRAY_SIZE-1;

		if (al_eth_kr_lt_all_not_updated(&report)) {
			ldcoeff.preset = AL_TRUE;

			nextstate = DO_PRESET;
		}

		break;
	case DO_PRESET:
		 
		if (!al_eth_kr_lt_all_not_updated(&report))
			nextstate = DO_HOLD;
		else  
			ldcoeff.preset = AL_TRUE;

		break;
	case DO_HOLD:
		 
		if (al_eth_kr_lt_all_not_updated(&report))
			nextstate = QMEASURE;
		break;

	case QMEASURE:
		 
		rc = al_serdes_eye_measure_run(kr_data->serdes_obj,
					       kr_data->grp,
					       kr_data->lane,
					       AL_ETH_KR_EYE_MEASURE_TIMEOUT,
					       &val);
		if (rc != 0) {
			al_warn("%s: Rx eye measurement failed\n", __func__);

			return rc;
		}

		al_dbg("%s: Rx Measure eye returned 0x%x\n", __func__, val);

		for (i = 0 ; i < QARRAY_SIZE-1 ; i++)
			kr_data->qarray[i] = kr_data->qarray[i+1];

		kr_data->qarray[QARRAY_SIZE-1] = val;

		if( kr_data->qarray_cnt < QARRAY_SIZE )
			kr_data->qarray_cnt++;

		nextstate = QCHECK;
		break;
	case QCHECK:
		 
		if (kr_data->qarray_cnt < QARRAY_SIZE) {
			 
			if(kr_data->coeff_status_step == C72_CSTATE_MIN)
				nextstate = COEFF_DONE;
			else {
				 
				al_eth_kr_lt_coef_set(&ldcoeff,
						      kr_data->curr_coeff,
						      AL_PHY_KR_COEF_UP_DEC);

				nextstate = DO_NEXT_TRY;
			}
		} else {
			 
			if ((kr_data->qarray[0] < kr_data->qarray[1]) &&
			    (kr_data->qarray[0] < kr_data->qarray[2])) {
				 
				al_eth_kr_lt_coef_set(&ldcoeff,
						      kr_data->curr_coeff,
						      AL_PHY_KR_COEF_UP_INC);

				nextstate = END_STEPS;
				if (kr_data->end_steps_cnt > 0)
					kr_data->end_steps_cnt--;
			} else {
				if(kr_data->coeff_status_step == C72_CSTATE_MIN)
					nextstate = COEFF_DONE;
				else {
					 
					al_eth_kr_lt_coef_set(&ldcoeff,
							      kr_data->curr_coeff,
							      AL_PHY_KR_COEF_UP_DEC);

					nextstate = DO_NEXT_TRY;
				}
			}
		}
		break;
	case DO_NEXT_TRY:
		 
		kr_data->coeff_status_step = coeff_status_cur;

		if (coeff_status_cur != C72_CSTATE_NOT_UPDATED)
			nextstate = DO_HOLD;   
		else
			al_eth_kr_lt_coef_set(&ldcoeff,
					      kr_data->curr_coeff,
					      AL_PHY_KR_COEF_UP_DEC);

		break;
	 
	case END_STEPS:
		if (coeff_status_cur != C72_CSTATE_NOT_UPDATED)
			nextstate = END_STEPS_HOLD;
		else
			al_eth_kr_lt_coef_set(&ldcoeff,
					      kr_data->curr_coeff,
					      AL_PHY_KR_COEF_UP_INC);

		break;
	case END_STEPS_HOLD:
		if (coeff_status_cur == C72_CSTATE_NOT_UPDATED) {
			if (kr_data->end_steps_cnt != 0) {
				 
				al_eth_kr_lt_coef_set(&ldcoeff,
						      kr_data->curr_coeff,
						      AL_PHY_KR_COEF_UP_INC);

				nextstate = END_STEPS;

				if (kr_data->end_steps_cnt > 0)
					kr_data->end_steps_cnt--;

			} else {
				nextstate = COEFF_DONE;
			}
		}
		break;
	case COEFF_DONE:
		 
		if(kr_data->curr_coeff < COEFF_TO_MANIPULATE_LAST) {
			int i;

			for (i = 0 ; i < QARRAY_SIZE ; i++)
					kr_data->qarray[i] = 0;

			kr_data->qarray_cnt = 0;
			kr_data->end_steps_cnt = QARRAY_SIZE-1;
			kr_data->coeff_status_step = C72_CSTATE_NOT_UPDATED;
			kr_data->curr_coeff++;

			al_dbg("[%s]: doing next coefficient: %d ---\n\n",
				kr_data->adapter->name, kr_data->curr_coeff);

			nextstate = QMEASURE;
		}
		else {
			nextstate = SET_READY;
		}
		break;
	case SET_READY:
		 
		kr_data->status_report.receiver_ready = AL_TRUE;
		 
		al_eth_receiver_ready_set(kr_data->adapter);

		nextstate = TX_DONE;
		break;
	case TX_DONE:
		break;   
	default:
		nextstate = kr_data->algo_state;
		break;
	}

	if(kr_data->algo_state != nextstate )
		al_dbg("[%s] [al_eth_kr_lt_transmit_run] STM changes %s -> %s: "
			" Qarray=%d/%d/%d\n", kr_data->adapter->name,
			al_eth_kr_mac_sm_name[kr_data->algo_state],
			al_eth_kr_mac_sm_name[nextstate],
			kr_data->qarray[0], kr_data->qarray[1], kr_data->qarray[2]);

	kr_data->algo_state = nextstate;

	al_eth_ld_coeff_up_set(kr_data->adapter, &ldcoeff);
	al_eth_ld_status_report_set(kr_data->adapter, &kr_data->status_report);

	return 0;
}

static int al_eth_kr_run_lt(struct al_eth_kr_data *kr_data)
{
	unsigned int	cnt;
	int		ret = 0;
	al_bool		page_received = AL_FALSE;
	al_bool		an_completed = AL_FALSE;
	al_bool		error = AL_FALSE;
	al_bool		training_failure = AL_FALSE;

	al_eth_kr_lt_initialize(kr_data->adapter);

	if (al_eth_kr_lt_frame_lock_wait(
		kr_data->adapter, AL_ETH_KR_FRAME_LOCK_TIMEOUT) == AL_TRUE) {
		 
		al_eth_kr_lt_receiver_task_init(kr_data);
		ret = al_eth_kr_lt_transmitter_task_init(kr_data);
		if (ret != 0)
			goto error;

		cnt = 0;
		do {
			ret = al_eth_kr_lt_receiver_task_run(kr_data);
			if (ret != 0)
				break;  

			ret = al_eth_kr_lt_transmitter_task_run(kr_data);
			if (ret != 0)
				break;   

			cnt++;
#ifdef CONFIG_SYNO_ALPINE_A0
			al_udelay(100);
#else
			al_udelay(1);
#endif

		} while((al_eth_kr_startup_proto_prog_get(kr_data->adapter)) &&
			 (cnt <= AL_ETH_KR_LT_MAX_ROUNDS));

		training_failure = al_eth_kr_training_status_fail_get(kr_data->adapter);
		al_dbg("[%s] training ended after %d rounds, failed = %s\n",
			kr_data->adapter->name, cnt,
			(training_failure) ? "Yes" : "No");
		if(training_failure || cnt > AL_ETH_KR_LT_MAX_ROUNDS) {
			al_info("[%s] Training Fail: status: %s, timeout: %s\n",
			       kr_data->adapter->name,
			       (training_failure) ? "Failed" : "OK",
			       (cnt > AL_ETH_KR_LT_MAX_ROUNDS) ? "Yes" : "No");
			 
			ret = -EIO;
			goto error;
		}

	} else {

		al_info("[%s] FAILED: did not achieve initial frame lock...\n",
			kr_data->adapter->name);

		ret = -EIO;
		goto error;
	}

	al_eth_kr_lt_stop(kr_data->adapter);

	cnt = AL_ETH_KR_LT_DONE_TIMEOUT;
	while (an_completed == AL_FALSE) {
		al_eth_kr_an_status_check(kr_data->adapter,
					  &page_received,
					  &an_completed,
					  &error);
		al_udelay(1);
		if ((cnt--) == 0) {
			al_info("%s: wait for an complete timeout!\n", __func__);
			ret = -ETIMEDOUT;
			goto error;
		}
	}

error:
	al_eth_kr_an_stop(kr_data->adapter);

	return ret;
}

int al_eth_an_lt_execute(struct al_hal_eth_adapter	*adapter,
			 struct al_serdes_obj		*serdes_obj,
			 enum al_serdes_group		grp,
			 enum al_serdes_lane		lane,
			 struct al_eth_an_adv		*an_adv,
			 struct al_eth_an_adv		*partner_adv)
{
	struct al_eth_kr_data		kr_data = {0};
	int				rc;
	struct al_serdes_adv_rx_params  rx_params;

	kr_data.adapter = adapter;
	kr_data.serdes_obj = serdes_obj;
	kr_data.grp = grp;
	kr_data.lane = lane;

	rx_params.override = AL_FALSE;
	al_serdes_rx_advanced_params_set(kr_data.serdes_obj,
					 kr_data.grp,
					 kr_data.lane,
					 &rx_params);

	rc = al_eth_kr_an_run(&kr_data, an_adv, partner_adv);
	if (rc) {
		al_eth_kr_lt_stop(adapter);
		al_eth_kr_an_stop(adapter);
		al_dbg("%s: auto-negotiation failed!\n", __func__);
		return rc;
	}

	if(partner_adv->technology != AL_ETH_AN_TECH_10GBASE_KR) {
		al_eth_kr_lt_stop(adapter);
		al_eth_kr_an_stop(adapter);
		al_dbg("%s: link partner isn't 10GBASE_KR.\n", __func__);
		return rc;
	}

	rc = al_eth_kr_run_lt(&kr_data);
	if (rc) {
		al_eth_kr_lt_stop(adapter);
		al_eth_kr_an_stop(adapter);
		al_dbg("%s: Link-training failed!\n", __func__);
		return rc;
	}

	return 0;
}
