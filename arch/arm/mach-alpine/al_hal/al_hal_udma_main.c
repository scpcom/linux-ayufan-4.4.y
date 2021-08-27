 
#include <al_hal_udma.h>

#define AL_UDMA_Q_RST_TOUT	10000	 

#define UDMA_STATE_IDLE		0x0
#define UDMA_STATE_NORMAL	0x1
#define UDMA_STATE_ABORT	0x2
#define UDMA_STATE_RESERVED	0x3

const char *const al_udma_states_name[] = {
	"Disable",
	"Idle",
	"Normal",
	"Abort",
	"Reset"
};

#define AL_UDMA_INITIAL_RING_ID	1

#define AL_UDMA_Q_FLAGS_IGNORE_RING_ID	AL_BIT(0)
#define AL_UDMA_Q_FLAGS_NO_COMP_UPDATE	AL_BIT(1)
#define AL_UDMA_Q_FLAGS_EN_COMP_COAL	AL_BIT(2)

static void al_udma_set_defaults(struct al_udma *udma)
{
	uint32_t tmp;

#if 0
	uint32_t reg, reg2;
#endif

	if (udma->type == UDMA_TX) {
		struct unit_regs* tmp_unit_regs =
			(struct unit_regs*)udma->udma_regs;

		tmp = al_reg_read32(
				&udma->udma_regs->m2s.m2s_rd.desc_pref_cfg_3);
		tmp &= ~ UDMA_M2S_RD_DESC_PREF_CFG_3_PREF_THR_MASK;
		tmp |= 16 << UDMA_M2S_RD_DESC_PREF_CFG_3_PREF_THR_SHIFT;
		al_reg_write32(&tmp_unit_regs->m2s.m2s_rd.desc_pref_cfg_3
									, tmp);
		al_reg_write32(& tmp_unit_regs->gen.axi.cfg_1,0);
		al_reg_write32(&tmp_unit_regs->m2s.m2s_comp.cfg_application_ack
					, 0);  

		tmp = al_reg_read32(&udma->udma_regs->m2s.axi_m2s.desc_wr_cfg_1);
		tmp &= ~ UDMA_AXI_M2S_DESC_WR_CFG_1_MAX_AXI_BEATS_MASK;
		tmp |= 4 << UDMA_AXI_M2S_DESC_WR_CFG_1_MAX_AXI_BEATS_SHIFT;
		al_reg_write32(&udma->udma_regs->m2s.axi_m2s.desc_wr_cfg_1
									, tmp);

	}
	if (udma->type == UDMA_RX) {
		al_reg_write32(
			&udma->udma_regs->s2m.s2m_comp.cfg_application_ack, 0);
					 
	}
#if 0
	if (udma->type == UDMA_TX) {
		reg = al_reg_read32(&udma->udma_regs->m2s.m2s_feature.reg_1);
		al_reg_write32(&udma->udma_regs->m2s.m2s_rd.desc_pref_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->m2s.m2s_feature.reg_3);
		al_reg_write32(&udma->udma_regs->m2s.m2s_rd.data_cfg, reg);

		reg = al_reg_read32(&udma->udma_regs->m2s.m2s_feature.reg_4);
		tmp = al_reg_read32(&udma->udma_regs->m2s.m2s_comp.cfg_1c);
		tmp &= ~0x1FFFF;
		tmp |= reg & 0x1FFFF;
		al_reg_write32(&udma->udma_regs->m2s.m2s_comp.cfg_1c, reg);

		reg = al_reg_read32(&udma->udma_regs->m2s.m2s_feature.reg_5);
		al_reg_write32(&udma->udma_regs->m2s.axi_m2s.ostand_cfg, reg);

		reg = al_reg_read32(&udma->udma_regs->m2s.axi_m2s.comp_wr_cfg_1);
		reg &= ~UDMA_AXI_M2S_COMP_WR_CFG_1_AWBURST_MASK;
		reg |= 0x1 << UDMA_AXI_M2S_COMP_WR_CFG_1_AWBURST_SHIFT;
		al_reg_write32(&udma->udma_regs->m2s.axi_m2s.comp_wr_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->m2s.axi_m2s.data_rd_cfg_1);
		reg &= ~UDMA_AXI_M2S_DATA_RD_CFG_1_ARBURST_MASK;
		reg |= 0x1 << UDMA_AXI_M2S_DATA_RD_CFG_1_ARBURST_SHIFT;
		al_reg_write32(&udma->udma_regs->m2s.axi_m2s.data_rd_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->m2s.axi_m2s.desc_rd_cfg_1);
		reg &= ~UDMA_AXI_M2S_DESC_RD_CFG_1_ARBURST_MASK;
		reg |= 0x1 << UDMA_AXI_M2S_DESC_RD_CFG_1_ARBURST_SHIFT;
		al_reg_write32(&udma->udma_regs->m2s.axi_m2s.desc_rd_cfg_1, reg);

	} else {
		reg = al_reg_read32(&udma->udma_regs->s2m.s2m_feature.reg_1);
		al_reg_write32(&udma->udma_regs->s2m.s2m_rd.desc_pref_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.s2m_feature.reg_3);
		al_reg_write32(&udma->udma_regs->s2m.s2m_wr.data_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.s2m_feature.reg_4);
		al_reg_write32(&udma->udma_regs->s2m.s2m_comp.cfg_2c, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.s2m_feature.reg_5);
		tmp = al_reg_read32(&udma->udma_regs->s2m.axi_s2m.ostand_cfg_rd);
		tmp &= ~UDMA_AXI_S2M_OSTAND_CFG_RD_MAX_DESC_RD_MASK;
		tmp |= AL_REG_FIELD_GET(reg,
					UDMA_S2M_FEATURE_REG_5_MAX_DESC_RD_OSTAND_MASK,
					UDMA_S2M_FEATURE_REG_5_MAX_DESC_RD_OSTAND_SHIFT);
		al_reg_write32(&udma->udma_regs->s2m.axi_s2m.ostand_cfg_rd, tmp);

		reg2 = al_reg_read32(&udma->udma_regs->s2m.s2m_feature.reg_6);

		tmp = AL_REG_FIELD_GET(reg,
					UDMA_S2M_FEATURE_REG_5_MAX_DATA_WR_OSTAND_MASK,
					UDMA_S2M_FEATURE_REG_5_MAX_DATA_WR_OSTAND_SHIFT);

		tmp |= AL_REG_FIELD_GET(reg2,
					UDMA_S2M_FEATURE_REG_6_MAX_DATA_BEATS_WR_OSTAND_MASK,
					UDMA_S2M_FEATURE_REG_6_MAX_DATA_BEATS_WR_OSTAND_SHIFT)
			<< UDMA_AXI_S2M_OSTAND_CFG_WR_MAX_DATA_DATA_WR_SHIFT;

		tmp |= AL_REG_FIELD_GET(reg2,
					UDMA_S2M_FEATURE_REG_5_MAX_COMP_REQ_MASK,
					UDMA_S2M_FEATURE_REG_5_MAX_COMP_REQ_SHIFT)
			<< UDMA_S2M_FEATURE_REG_5_MAX_COMP_REQ_SHIFT;

		tmp |= AL_REG_FIELD_GET(reg2,
					UDMA_S2M_FEATURE_REG_5_MAX_COMP_DATA_WR_OSTAND_MASK,
					UDMA_S2M_FEATURE_REG_5_MAX_COMP_DATA_WR_OSTAND_SHIFT)
			<< UDMA_AXI_S2M_OSTAND_CFG_WR_MAX_COMP_DATA_WR_SHIFT;

		al_reg_write32(&udma->udma_regs->s2m.axi_s2m.ostand_cfg_wr, tmp);

		tmp = al_reg_read32(&udma->udma_regs->s2m.s2m_comp.cfg_1c);
		tmp &= ~UDMA_S2M_COMP_CFG_1C_ACK_FIFO_DEPTH_MASK;
		tmp |= AL_REG_FIELD_GET(reg2,
					UDMA_S2M_FEATURE_REG_6_MAX_STREAM_ACK_MASK,
					UDMA_S2M_FEATURE_REG_6_MAX_STREAM_ACK_SHIFT) << UDMA_S2M_COMP_CFG_1C_ACK_FIFO_DEPTH_SHIFT; 
		al_reg_write32(&udma->udma_regs->s2m.s2m_comp.cfg_1c, tmp);

		reg = al_reg_read32(&udma->udma_regs->s2m.axi_s2m.comp_wr_cfg_1);
		reg &= ~UDMA_AXI_S2M_COMP_WR_CFG_1_AWBURST_MASK;
		reg |= 0x1 << UDMA_AXI_S2M_COMP_WR_CFG_1_AWBURST_SHIFT;
		reg &= ~UDMA_AXI_S2M_COMP_WR_CFG_1_AWID_MASK;
		reg |= 0x2 << UDMA_AXI_S2M_COMP_WR_CFG_1_AWID_SHIFT;
		al_reg_write32(&udma->udma_regs->s2m.axi_s2m.comp_wr_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.axi_s2m.desc_rd_cfg_4);
		reg &= ~UDMA_AXI_S2M_DESC_RD_CFG_4_ARBURST_MASK;
		reg |= 0x1<<UDMA_AXI_S2M_DESC_RD_CFG_4_ARBURST_SHIFT;
		al_reg_write32(&udma->udma_regs->s2m.axi_s2m.desc_rd_cfg_4, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.axi_s2m.data_wr_cfg_1);
		reg &= ~UDMA_AXI_S2M_DATA_WR_CFG_1_AWBURST_MASK;
		reg |= 0x1<<UDMA_AXI_S2M_DATA_WR_CFG_1_AWBURST_SHIFT;
		reg &= ~UDMA_AXI_S2M_DATA_WR_CFG_1_AWID_MASK;
		reg |= 0x3<<UDMA_AXI_S2M_DATA_WR_CFG_1_AWID_SHIFT;
		al_reg_write32(&udma->udma_regs->s2m.axi_s2m.data_wr_cfg_1, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.s2m_comp.cfg_2c);
		reg &= ~UDMA_S2M_COMP_CFG_2C_UNACK_FIFO_DEPTH_MASK;
		reg |= 0x20<<UDMA_S2M_COMP_CFG_2C_UNACK_FIFO_DEPTH_SHIFT;
		al_reg_write32(&udma->udma_regs->s2m.s2m_comp.cfg_2c, reg);

		reg = al_reg_read32(&udma->udma_regs->s2m.axi_s2m.ostand_cfg_wr);
		reg &= ~UDMA_AXI_S2M_OSTAND_CFG_WR_MAX_COMP_DATA_WR_MASK;
		reg |= 0x20<<UDMA_AXI_S2M_OSTAND_CFG_WR_MAX_COMP_DATA_WR_SHIFT;
		al_reg_write32(&udma->udma_regs->s2m.axi_s2m.ostand_cfg_wr, reg);

		al_reg_write32(&udma->udma_regs->s2m.s2m.err_abort_mask, 1<<24);
	}
#endif
}
 
static int al_udma_q_config(struct al_udma_q *udma_q)
{
	uint32_t *reg_addr;
	uint32_t val;

	if (udma_q->udma->type == UDMA_TX) {
		reg_addr = &udma_q->q_regs->m2s_q.rlimit.mask;

		val = al_reg_read32(reg_addr);
		 
		val &= ~UDMA_M2S_Q_RATE_LIMIT_MASK_INTERNAL_PAUSE_DMB;
		al_reg_write32(reg_addr, val);
	}
	return 0;
}

static int al_udma_q_config_compl(struct al_udma_q *udma_q)
{
	uint32_t *reg_addr;
	uint32_t val;

	if (udma_q->udma->type == UDMA_TX)
		reg_addr = &udma_q->q_regs->m2s_q.comp_cfg;
	else
		reg_addr = &udma_q->q_regs->s2m_q.comp_cfg;

	val = al_reg_read32(reg_addr);

	if (udma_q->flags & AL_UDMA_Q_FLAGS_NO_COMP_UPDATE)
		val &= ~UDMA_M2S_Q_COMP_CFG_EN_COMP_RING_UPDATE;
	else
		val |= UDMA_M2S_Q_COMP_CFG_EN_COMP_RING_UPDATE;

	if (udma_q->flags & AL_UDMA_Q_FLAGS_EN_COMP_COAL)
		val &= ~UDMA_M2S_Q_COMP_CFG_DIS_COMP_COAL;
	else
		val |= UDMA_M2S_Q_COMP_CFG_DIS_COMP_COAL;

	al_reg_write32(reg_addr, val);

	if (udma_q->udma->type == UDMA_RX) {
		val = al_reg_read32(
				&udma_q->udma->udma_regs->s2m.s2m_comp.cfg_1c);
		val &= ~UDMA_S2M_COMP_CFG_1C_DESC_SIZE_MASK;
		 
		val |= (udma_q->cdesc_size >> 2)
				& UDMA_S2M_COMP_CFG_1C_DESC_SIZE_MASK;
		al_reg_write32(&udma_q->udma->udma_regs->s2m.s2m_comp.cfg_1c
							, val);
	}
	return 0;
}

static int al_udma_q_set_pointers(struct al_udma_q *udma_q)
{
	 
	al_assert((AL_ADDR_LOW(udma_q->desc_phy_base) &
		   ~UDMA_M2S_Q_TDRBP_LOW_ADDR_MASK) == 0);
	al_reg_write32(&udma_q->q_regs->rings.drbp_low,
		       AL_ADDR_LOW(udma_q->desc_phy_base));
	al_reg_write32(&udma_q->q_regs->rings.drbp_high,
		       AL_ADDR_HIGH(udma_q->desc_phy_base));

	al_reg_write32(&udma_q->q_regs->rings.drl, udma_q->size);

	if (udma_q->cdesc_base_ptr == NULL) {
		udma_q->flags |= AL_UDMA_Q_FLAGS_NO_COMP_UPDATE;
	} else {
		 
		al_assert((AL_ADDR_LOW(udma_q->cdesc_phy_base) &
			   ~UDMA_M2S_Q_TCRBP_LOW_ADDR_MASK) == 0);
		al_reg_write32(&udma_q->q_regs->rings.crbp_low,
			       AL_ADDR_LOW(udma_q->cdesc_phy_base));
		al_reg_write32(&udma_q->q_regs->rings.crbp_high,
			       AL_ADDR_HIGH(udma_q->cdesc_phy_base));

		al_udma_q_config_compl(udma_q);
	}
	return 0;
}

static int al_udma_q_enable(struct al_udma_q *udma_q, int enable)
{
	uint32_t reg = al_reg_read32(&udma_q->q_regs->rings.cfg);

	if (enable) {
		reg |= (UDMA_M2S_Q_CFG_EN_PREF | UDMA_M2S_Q_CFG_EN_SCHEDULING);
		udma_q->status = AL_QUEUE_ENABLED;
	} else {
		reg &= ~(UDMA_M2S_Q_CFG_EN_PREF | UDMA_M2S_Q_CFG_EN_SCHEDULING);
		udma_q->status = AL_QUEUE_DISABLED;
	}
	al_reg_write32(&udma_q->q_regs->rings.cfg, reg);
	return 0;
}

int al_udma_init(struct al_udma *udma, struct al_udma_params *udma_params)
{
	int i;

	al_assert(udma);

	if (udma_params->num_of_queues > DMA_MAX_Q) {
		al_err("udma: invalid num_of_queues parameter\n");
		return -EINVAL;
	}

	udma->udma_regs = udma_params->udma_reg;
	udma->type = udma_params->type;
	udma->num_of_queues = udma_params->num_of_queues;

	if (udma_params->name == NULL)
		udma->name = "";
	else
		udma->name = udma_params->name;

	udma->state = UDMA_DISABLE;
	for (i = 0; i < DMA_MAX_Q; i++) {
		udma->udma_q[i].status = AL_QUEUE_NOT_INITIALIZED;
	}
	 
	al_udma_set_defaults(udma);
	al_dbg("udma [%s] initialized. base %p\n", udma->name,
		udma->udma_regs);
	return 0;
}

int al_udma_q_init(struct al_udma *udma, uint32_t qid,
					struct al_udma_q_params *q_params)
{
	struct al_udma_q *udma_q;

	al_assert(udma);
	al_assert(q_params);

	if (qid >= udma->num_of_queues) {
		al_err("udma: invalid queue id (%d)\n", qid);
		return -EINVAL;
	}

	if (udma->udma_q[qid].status == AL_QUEUE_ENABLED) {
		al_err("udma: queue (%d) already enabled!\n", qid);
		return -EIO;
	}

	if (q_params->size < AL_UDMA_MIN_Q_SIZE) {
		al_err("udma: queue (%d) size too small\n", qid);
		return -EINVAL;
	}

	if (q_params->size > AL_UDMA_MAX_Q_SIZE) {
		al_err("udma: queue (%d) size too large\n", qid);
		return -EINVAL;
	}

	if (q_params->size & (q_params->size - 1)) {
		al_err("udma: queue (%d) size (%d) must be power of 2\n",
			 q_params->size, qid);
		return -EINVAL;
	}

	udma_q = &udma->udma_q[qid];
	 
	if (udma->type == UDMA_TX)
		udma_q->q_regs = (union udma_q_regs __iomem *)
					&udma->udma_regs->m2s.m2s_q[qid];
	else
		udma_q->q_regs = (union udma_q_regs __iomem *)
					&udma->udma_regs->s2m.s2m_q[qid];

	udma_q->dev_id = q_params->dev_id;
	udma_q->rev_id = q_params->rev_id;
	udma_q->size = q_params->size;
	udma_q->size_mask = q_params->size - 1;
	udma_q->desc_base_ptr = q_params->desc_base;
	udma_q->desc_phy_base = q_params->desc_phy_base;
	udma_q->cdesc_base_ptr = q_params->cdesc_base;
	udma_q->cdesc_phy_base = q_params->cdesc_phy_base;
	udma_q->cdesc_size = q_params->cdesc_size;

	udma_q->next_desc_idx = 0;
	udma_q->next_cdesc_idx = 0;
	udma_q->end_cdesc_ptr = (uint8_t *) udma_q->cdesc_base_ptr +
	    (udma_q->size - 1) * udma_q->cdesc_size;
	udma_q->comp_head_idx = 0;
	udma_q->comp_head_ptr = (union al_udma_cdesc *)udma_q->cdesc_base_ptr;
	udma_q->desc_ring_id = AL_UDMA_INITIAL_RING_ID;
	udma_q->comp_ring_id = AL_UDMA_INITIAL_RING_ID;
#if 0
	udma_q->desc_ctrl_bits = AL_UDMA_INITIAL_RING_ID <<
						AL_M2S_DESC_RING_ID_SHIFT;
#endif
	udma_q->pkt_crnt_descs = 0;
	udma_q->flags = 0;
	udma_q->status = AL_QUEUE_DISABLED;
	udma_q->udma = udma;
	udma_q->qid = qid;

	al_udma_q_config(udma_q);
	 
	al_udma_q_set_pointers(udma_q);

	al_udma_q_enable(udma_q, 1);

	al_dbg("udma [%s %d]: %s q init. size 0x%x\n"
			"  desc ring info: phys base 0x%llx virt base %p\n"
			"  cdesc ring info: phys base 0x%llx virt base %p "
				"entry size 0x%x",
			udma_q->udma->name, udma_q->qid,
			udma->type == UDMA_TX ? "Tx" : "Rx",
			q_params->size,
			(unsigned long long)q_params->desc_phy_base,
			q_params->desc_base,
			(unsigned long long)q_params->cdesc_phy_base,
			q_params->cdesc_base,
			q_params->cdesc_size);

	return 0;
}

int al_udma_q_reset(struct al_udma_q *udma_q)
{
	unsigned int remaining_time = AL_UDMA_Q_RST_TOUT;
	uint32_t *status_reg;
	uint32_t *dcp_reg;
	uint32_t *crhp_reg;
	uint32_t *q_sw_ctrl_reg;

	al_assert(udma_q);

	al_udma_q_enable(udma_q, 0);

	status_reg = &udma_q->q_regs->rings.status;

	while (remaining_time) {
		uint32_t status = al_reg_read32(status_reg);

		if (!(status & (UDMA_M2S_Q_STATUS_PREFETCH |
						UDMA_M2S_Q_STATUS_SCHEDULER)))
			break;

		remaining_time--;
		al_udelay(1);
	}

	if (!remaining_time) {
		al_err("udma [%s %d]: %s timeout waiting for prefetch and "
			"scheduler disable\n", udma_q->udma->name, udma_q->qid,
			__func__);
		return -ETIME;
	}

	dcp_reg = &udma_q->q_regs->rings.dcp;
	crhp_reg = &udma_q->q_regs->rings.crhp;

	while (remaining_time) {
		uint32_t dcp = al_reg_read32(dcp_reg);
		uint32_t crhp = al_reg_read32(crhp_reg);

		if (dcp == crhp)
			break;

		remaining_time--;
		al_udelay(1);
	};

	if (!remaining_time) {
		al_err("udma [%s %d]: %s timeout waiting for dcp==crhp\n",
			udma_q->udma->name, udma_q->qid, __func__);
		return -ETIME;
	}

	if (udma_q->udma->type == UDMA_TX)
		q_sw_ctrl_reg = &udma_q->q_regs->m2s_q.q_sw_ctrl;
	else
		q_sw_ctrl_reg = &udma_q->q_regs->s2m_q.q_sw_ctrl;

	al_reg_write32(q_sw_ctrl_reg, UDMA_M2S_Q_SW_CTRL_RST_Q);

	return 0;
}

int al_udma_q_handle_get(struct al_udma *udma, uint32_t qid,
						struct al_udma_q **q_handle)
{

	al_assert(udma);
	al_assert(q_handle);

	if (unlikely(qid >= udma->num_of_queues)) {
		al_err("udma [%s]: invalid queue id (%d)\n", udma->name, qid);
		return -EINVAL;
	}
	*q_handle = &udma->udma_q[qid];
	return 0;
}

int al_udma_state_set(struct al_udma *udma, enum al_udma_state state)
{
	uint32_t reg;

	al_assert(udma != NULL);
	if (state == udma->state)
		al_dbg("udma [%s]: requested state identical to "
			"current state (%d)\n", udma->name, state);

	al_dbg("udma [%s]: change state from (%s) to (%s)\n",
		 udma->name, al_udma_states_name[udma->state],
		 al_udma_states_name[state]);

	reg = 0;
	switch (state) {
	case UDMA_DISABLE:
		reg |= UDMA_M2S_CHANGE_STATE_DIS;
		break;
	case UDMA_NORMAL:
		reg |= UDMA_M2S_CHANGE_STATE_NORMAL;
		break;
	case UDMA_ABORT:
		reg |= UDMA_M2S_CHANGE_STATE_ABORT;
		break;
	default:
		al_err("udma: invalid state (%d)\n", state);
		return -EINVAL;
	}

	if (udma->type == UDMA_TX)
		al_reg_write32(&udma->udma_regs->m2s.m2s.change_state, reg);
	else
		al_reg_write32(&udma->udma_regs->s2m.s2m.change_state, reg);

	udma->state = state;
	return 0;
}

enum al_udma_state al_udma_state_get(struct al_udma *udma)
{
	uint32_t state_reg;
	uint32_t comp_ctrl;
	uint32_t stream_if;
	uint32_t data_rd;
	uint32_t desc_pref;

	if (udma->type == UDMA_TX)
		state_reg = al_reg_read32(&udma->udma_regs->m2s.m2s.state);
	else
		state_reg = al_reg_read32(&udma->udma_regs->s2m.s2m.state);

	comp_ctrl = AL_REG_FIELD_GET(state_reg,
				     UDMA_M2S_STATE_COMP_CTRL_MASK,
				     UDMA_M2S_STATE_COMP_CTRL_SHIFT);
	stream_if = AL_REG_FIELD_GET(state_reg,
				     UDMA_M2S_STATE_STREAM_IF_MASK,
				     UDMA_M2S_STATE_STREAM_IF_SHIFT);
	data_rd = AL_REG_FIELD_GET(state_reg,
				   UDMA_M2S_STATE_DATA_RD_CTRL_MASK,
				   UDMA_M2S_STATE_DATA_RD_CTRL_SHIFT);
	desc_pref = AL_REG_FIELD_GET(state_reg,
				     UDMA_M2S_STATE_DESC_PREF_MASK,
				     UDMA_M2S_STATE_DESC_PREF_SHIFT);

	al_assert(comp_ctrl != UDMA_STATE_RESERVED);
	al_assert(stream_if != UDMA_STATE_RESERVED);
	al_assert(data_rd != UDMA_STATE_RESERVED);
	al_assert(desc_pref != UDMA_STATE_RESERVED);

	if ((comp_ctrl == UDMA_STATE_ABORT) || (stream_if == UDMA_STATE_ABORT)
			|| (data_rd == UDMA_STATE_ABORT)
			|| (desc_pref == UDMA_STATE_ABORT))
		return UDMA_ABORT;

	if ((comp_ctrl == UDMA_STATE_NORMAL)
			|| (stream_if == UDMA_STATE_NORMAL)
			|| (data_rd == UDMA_STATE_NORMAL)
			|| (desc_pref == UDMA_STATE_NORMAL))
		return UDMA_NORMAL;

	return UDMA_IDLE;
}

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
 
#else
#define cdesc_is_first(flags) ((flags) & AL_UDMA_CDESC_FIRST)
#define cdesc_is_last(flags) ((flags) & AL_UDMA_CDESC_LAST)

static INLINE al_bool al_udma_new_cdesc(struct al_udma_q *udma_q,
								uint32_t flags)
{
	if (((flags & AL_M2S_DESC_RING_ID_MASK) >> AL_M2S_DESC_RING_ID_SHIFT)
	    == udma_q->comp_ring_id)
		return AL_TRUE;
	return AL_FALSE;
}

static INLINE union al_udma_cdesc *al_cdesc_next_update(
						struct al_udma_q *udma_q,
						union al_udma_cdesc *cdesc)
{
	 
	if (unlikely(((uint8_t *) cdesc == udma_q->end_cdesc_ptr))) {
		udma_q->comp_ring_id =
		    (udma_q->comp_ring_id + 1) & DMA_RING_ID_MASK;
		return (union al_udma_cdesc *) udma_q->cdesc_base_ptr;
	}
	return (union al_udma_cdesc *) ((uint8_t *) cdesc + udma_q->cdesc_size);
}
#endif

#ifdef CONFIG_SYNO_ALPINE_V2_5_3
uint32_t al_udma_cdesc_packet_get(
	struct al_udma_q		*udma_q,
	volatile union al_udma_cdesc	**cdesc)
#else
uint32_t al_udma_cdesc_packet_get(struct al_udma_q *udma_q,
					union al_udma_cdesc **cdesc)
#endif
{
	uint32_t count;
#ifdef CONFIG_SYNO_ALPINE_V2_5_3
	volatile union al_udma_cdesc *curr;
#else
	union al_udma_cdesc *curr;
#endif
	uint32_t comp_flags;

	al_assert(!(udma_q->flags & AL_UDMA_Q_FLAGS_NO_COMP_UPDATE));

	curr = udma_q->comp_head_ptr;
	comp_flags = swap32_from_le(curr->al_desc_comp_tx.ctrl_meta);

	if (unlikely(al_udma_new_cdesc(udma_q, comp_flags) == AL_FALSE))
		return 0;
	 
	count = udma_q->pkt_crnt_descs + 1;
	while (!cdesc_is_last(comp_flags)) {
		curr = al_cdesc_next_update(udma_q, curr);
		comp_flags = swap32_from_le(curr->al_desc_comp_tx.ctrl_meta);
		if (unlikely(al_udma_new_cdesc(udma_q, comp_flags)
								== AL_FALSE)) {
			 
			udma_q->pkt_crnt_descs = count;
			udma_q->comp_head_ptr = curr;
			return 0;
		}
		count++;
		 
		al_assert(count <= udma_q->size);
	}
	 
	*cdesc = al_udma_cdesc_idx_to_ptr(udma_q, udma_q->next_cdesc_idx);
	udma_q->pkt_crnt_descs = 0;
	udma_q->comp_head_ptr = al_cdesc_next_update(udma_q, curr);

	al_dbg("udma [%s %d]: packet completed. first desc %p (ixd 0x%x)"
		 " descs %d\n", udma_q->udma->name, udma_q->qid, *cdesc,
		 udma_q->next_cdesc_idx, count);

	return count;
}
