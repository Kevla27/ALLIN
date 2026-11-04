#include "ami_ade9000.h"
#include <string.h>

int ami_ade9000_init(AmiAde9000Context *ctx,
                     const AmiAde9000Calibration *cal,
                     AmiAde9000SpiTransferFunc spi_func,
                     void *user_data)
{
    if (ctx == NULL) {
        return -1;
    }

    memset(ctx, 0, sizeof(*ctx));

    if (cal != NULL) {
        ctx->cal = *cal;
    } else {
        ctx->cal.v_scale_num = ADE9000_DEFAULT_V_SCALE_NUM;
        ctx->cal.v_scale_den = ADE9000_DEFAULT_V_SCALE_DEN;
        ctx->cal.i_scale_num = ADE9000_DEFAULT_I_SCALE_NUM;
        ctx->cal.i_scale_den = ADE9000_DEFAULT_I_SCALE_DEN;
        ctx->cal.wh_per_code = ADE9000_DEFAULT_WH_PER_PULSE;
        ctx->cal.varh_per_code = ADE9000_DEFAULT_WH_PER_PULSE;
    }

    /* Guard against divide by zero in calibration constants */
    if (ctx->cal.v_scale_den == 0) ctx->cal.v_scale_den = 1;
    if (ctx->cal.i_scale_den == 0) ctx->cal.i_scale_den = 1;
    if (ctx->cal.wh_per_code == 0) ctx->cal.wh_per_code = 1;
    if (ctx->cal.varh_per_code == 0) ctx->cal.varh_per_code = 1;

    ctx->spi_transfer = spi_func;
    ctx->user_data = user_data;
    ctx->is_initialized = 1;

    return 0;
}

int ami_ade9000_read_reg(AmiAde9000Context *ctx, uint16_t reg_addr, uint32_t *val)
{
    if (ctx == NULL || !ctx->is_initialized || val == NULL) {
        return -1;
    }

    if (ctx->spi_transfer != NULL) {
        return ctx->spi_transfer(reg_addr, 1u, val, ctx->user_data);
    }

    return -2;
}

int ami_ade9000_write_reg(AmiAde9000Context *ctx, uint16_t reg_addr, uint32_t val)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return -1;
    }

    if (ctx->spi_transfer != NULL) {
        uint32_t data = val;
        return ctx->spi_transfer(reg_addr, 0u, &data, ctx->user_data);
    }

    return -2;
}

int ami_ade9000_acquire_all(AmiAde9000Context *ctx)
{
    if (ctx == NULL || !ctx->is_initialized) {
        return AMI_ADE9000_ERR_NOT_INIT;
    }

    int err = 0;

    /* Voltages RMS */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AVRMS, &ctx->raw.vrms_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BVRMS, &ctx->raw.vrms_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CVRMS, &ctx->raw.vrms_raw[2]);

    /* Currents RMS */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AIRMS, &ctx->raw.irms_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BIRMS, &ctx->raw.irms_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CIRMS, &ctx->raw.irms_raw[2]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_NIRMS, &ctx->raw.irms_neutral_raw);

    /* Active Power */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AWATT, (uint32_t *)&ctx->raw.watt_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BWATT, (uint32_t *)&ctx->raw.watt_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CWATT, (uint32_t *)&ctx->raw.watt_raw[2]);

    /* Reactive Power */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AVAR, (uint32_t *)&ctx->raw.var_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BVAR, (uint32_t *)&ctx->raw.var_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CVAR, (uint32_t *)&ctx->raw.var_raw[2]);

    /* Apparent Power */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AVA, (uint32_t *)&ctx->raw.va_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BVA, (uint32_t *)&ctx->raw.va_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CVA, (uint32_t *)&ctx->raw.va_raw[2]);

    /* Power Factor */
    uint32_t tmp = 0;
    if (ami_ade9000_read_reg(ctx, ADE9000_REG_APF, &tmp) == 0) ctx->raw.pf_raw[0] = (int16_t)tmp; else err |= -1;
    if (ami_ade9000_read_reg(ctx, ADE9000_REG_BPF, &tmp) == 0) ctx->raw.pf_raw[1] = (int16_t)tmp; else err |= -1;
    if (ami_ade9000_read_reg(ctx, ADE9000_REG_CPF, &tmp) == 0) ctx->raw.pf_raw[2] = (int16_t)tmp; else err |= -1;

    /* Period / Frequency */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_APERIOD, &ctx->raw.period_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BPERIOD, &ctx->raw.period_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CPERIOD, &ctx->raw.period_raw[2]);

    /* Energy Accumulators */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AWATTHR_HI, &ctx->raw.watthr_hi_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BWATTHR_HI, &ctx->raw.watthr_hi_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CWATTHR_HI, &ctx->raw.watthr_hi_raw[2]);

    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AFVARHR_HI, &ctx->raw.fvarhr_hi_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BFVARHR_HI, &ctx->raw.fvarhr_hi_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CFVARHR_HI, &ctx->raw.fvarhr_hi_raw[2]);

    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_AVAHR_HI, &ctx->raw.vahr_hi_raw[0]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_BVAHR_HI, &ctx->raw.vahr_hi_raw[1]);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_CVAHR_HI, &ctx->raw.vahr_hi_raw[2]);

    /* Status & Events */
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_STATUS0, &ctx->raw.status0);
    err |= ami_ade9000_read_reg(ctx, ADE9000_REG_STATUS1, &ctx->raw.status1);

    if (err != 0) {
        ctx->scaled.is_valid = 0u;
        ctx->scaled.last_error_code = AMI_ADE9000_ERR_SPI;
        return AMI_ADE9000_ERR_SPI;
    }

    ctx->scaled.is_valid = 1u;
    ctx->scaled.last_error_code = AMI_ADE9000_OK;
    ami_ade9000_process_scaling(ctx);
    return AMI_ADE9000_OK;
}

void ami_ade9000_process_scaling(AmiAde9000Context *ctx)
{
    if (ctx == NULL) {
        return;
    }

    /* 1. Voltages: mV = (raw * v_scale_num) / v_scale_den */
    for (int i = 0; i < 3; i++) {
        uint64_t calc = ((uint64_t)ctx->raw.vrms_raw[i] * ctx->cal.v_scale_num) / ctx->cal.v_scale_den;
        ctx->scaled.phase_voltage_mv[i] = (uint32_t)calc;
    }

    /* 2. Currents: mA = (raw * i_scale_num) / i_scale_den */
    for (int i = 0; i < 3; i++) {
        uint64_t calc = ((uint64_t)ctx->raw.irms_raw[i] * ctx->cal.i_scale_num) / ctx->cal.i_scale_den;
        ctx->scaled.phase_current_ma[i] = (uint32_t)calc;
    }
    uint64_t n_calc = ((uint64_t)ctx->raw.irms_neutral_raw * ctx->cal.i_scale_num) / ctx->cal.i_scale_den;
    ctx->scaled.neutral_current_ma = (uint32_t)n_calc;

    /* 3. Powers */
    ctx->scaled.total_active_power_w = 0;
    ctx->scaled.total_reactive_power_var = 0;
    ctx->scaled.total_apparent_power_va = 0;

    for (int i = 0; i < 3; i++) {
        /* P = (V_mV * I_mA) / 1,000,000 W or direct scaling */
        int64_t p_calc = ((int64_t)ctx->raw.watt_raw[i] * 1150) / 20638835;
        ctx->scaled.active_power_w[i] = (int32_t)p_calc;
        ctx->scaled.total_active_power_w += ctx->scaled.active_power_w[i];

        int64_t q_calc = ((int64_t)ctx->raw.var_raw[i] * 1150) / 20638835;
        ctx->scaled.reactive_power_var[i] = (int32_t)q_calc;
        ctx->scaled.total_reactive_power_var += ctx->scaled.reactive_power_var[i];

        uint64_t s_calc = ((uint64_t)ctx->raw.va_raw[i] * 1150) / 20638835;
        ctx->scaled.apparent_power_va[i] = (uint32_t)s_calc;
        ctx->scaled.total_apparent_power_va += ctx->scaled.apparent_power_va[i];

        ctx->scaled.power_factor[i] = ctx->raw.pf_raw[i];
    }

    /* 4. Frequency (Period to mHz, 50.00 Hz nominal) */
    if (ctx->raw.period_raw[0] > 0) {
        /* Period register nominal ~ 160000 counts @ 50 Hz */
        uint64_t f_calc = (8000ULL * 1000ULL * 1000ULL) / (ctx->raw.period_raw[0] + 1);
        if (f_calc >= 40000 && f_calc <= 70000) {
            ctx->scaled.frequency_mhz = (uint32_t)f_calc;
        } else {
            ctx->scaled.frequency_mhz = 50000; /* fallback 50 Hz */
        }
    } else {
        ctx->scaled.frequency_mhz = 50000;
    }

    /* 5. Energy Accumulation (Wh) */
    uint64_t total_active_wh = 0;
    uint64_t total_reactive_varh = 0;
    for (int i = 0; i < 3; i++) {
        total_active_wh += (uint64_t)ctx->raw.watthr_hi_raw[i] * ctx->cal.wh_per_code;
        total_reactive_varh += (uint64_t)ctx->raw.fvarhr_hi_raw[i] * ctx->cal.varh_per_code;
    }
    ctx->scaled.active_import_wh = total_active_wh;
    ctx->scaled.reactive_import_varh = total_reactive_varh;

    /* 6. Tamper & Event Detection */
    ctx->scaled.reverse_power_flags = 0;
    if (ctx->raw.status0 & ADE9000_STATUS0_REVAPA) ctx->scaled.reverse_power_flags |= (1u << 0);
    if (ctx->raw.status0 & ADE9000_STATUS0_REVAPB) ctx->scaled.reverse_power_flags |= (1u << 1);
    if (ctx->raw.status0 & ADE9000_STATUS0_REVAPC) ctx->scaled.reverse_power_flags |= (1u << 2);

    ctx->scaled.phase_sequence_error = (ctx->raw.status0 & ADE9000_STATUS0_PHASE_SEQ) ? 1u : 0u;

    ctx->scaled.loss_of_phase_flags = 0;
    if (ctx->raw.status1 & ADE9000_STATUS1_ZXTOA) ctx->scaled.loss_of_phase_flags |= (1u << 0);
    if (ctx->raw.status1 & ADE9000_STATUS1_ZXTOB) ctx->scaled.loss_of_phase_flags |= (1u << 1);
    if (ctx->raw.status1 & ADE9000_STATUS1_ZXTOC) ctx->scaled.loss_of_phase_flags |= (1u << 2);
}

int ami_ade9000_feed_cosem_snapshot(const AmiAde9000Context *ctx,
                                   AmiCosemRegisterSnapshot *snapshot)
{
    if (ctx == NULL || snapshot == NULL) {
        return AMI_ADE9000_ERR_PARAM;
    }
    if (!ctx->is_initialized) {
        return AMI_ADE9000_ERR_NOT_INIT;
    }
    if (!ctx->scaled.is_valid) {
        return AMI_ADE9000_ERR_INVALID_DATA;
    }

    /* Populate phase voltages and currents */
    for (int i = 0; i < 3; i++) {
        snapshot->phase_voltage_millivolts[i] = ctx->scaled.phase_voltage_mv[i];
        snapshot->phase_current_milliamps[i] = ctx->scaled.phase_current_ma[i];
    }

    /* Populate energy registers */
    snapshot->active_import_wh = ctx->scaled.active_import_wh;
    snapshot->active_export_wh = ctx->scaled.active_export_wh;
    snapshot->reactive_import_varh = ctx->scaled.reactive_import_varh;
    snapshot->reactive_export_varh = ctx->scaled.reactive_export_varh;

    /* Enable valid masks for populated registers */
    snapshot->valid_mask |= (AMI_COSEM_VALID_VOLTAGE_A |
                             AMI_COSEM_VALID_VOLTAGE_B |
                             AMI_COSEM_VALID_VOLTAGE_C |
                             AMI_COSEM_VALID_CURRENT_A |
                             AMI_COSEM_VALID_CURRENT_B |
                             AMI_COSEM_VALID_CURRENT_C |
                             AMI_COSEM_VALID_ACTIVE_IMPORT |
                             AMI_COSEM_VALID_ACTIVE_EXPORT |
                             AMI_COSEM_VALID_REACTIVE_IMPORT |
                             AMI_COSEM_VALID_REACTIVE_EXPORT);

    /* Check for Tamper events to record into Event Log (SPLN D3.006-2:2021) */
    if (ctx->scaled.reverse_power_flags != 0 && snapshot->event_log_count < AMI_COSEM_PROFILE_MAX_ENTRIES) {
        /* Event 27: Reverse Power 3-Phase */
        snapshot->event_log[snapshot->event_log_count].event_code = 27u;
        snapshot->event_log[snapshot->event_log_count].timestamp = snapshot->clock_date_time;
        snapshot->event_log[snapshot->event_log_count].status = (uint32_t)ctx->raw.status0;
        snapshot->event_log_count++;
        snapshot->valid_mask |= AMI_COSEM_VALID_EVENT_LOG;
    }
    if (ctx->scaled.phase_sequence_error && snapshot->event_log_count < AMI_COSEM_PROFILE_MAX_ENTRIES) {
        /* Event 22: Phase Sequence Reverse */
        snapshot->event_log[snapshot->event_log_count].event_code = 22u;
        snapshot->event_log[snapshot->event_log_count].timestamp = snapshot->clock_date_time;
        snapshot->event_log[snapshot->event_log_count].status = (uint32_t)ctx->raw.status0;
        snapshot->event_log_count++;
        snapshot->valid_mask |= AMI_COSEM_VALID_EVENT_LOG;
    }
    if (ctx->scaled.loss_of_phase_flags != 0 && snapshot->event_log_count < AMI_COSEM_PROFILE_MAX_ENTRIES) {
        /* Event 25: Loss of Phase */
        snapshot->event_log[snapshot->event_log_count].event_code = 25u;
        snapshot->event_log[snapshot->event_log_count].timestamp = snapshot->clock_date_time;
        snapshot->event_log[snapshot->event_log_count].status = (uint32_t)ctx->raw.status0;
        snapshot->event_log_count++;
        snapshot->valid_mask |= AMI_COSEM_VALID_EVENT_LOG;
    }

    return 0;
}

