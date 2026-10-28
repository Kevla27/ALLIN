/**
 * @file e1_metrology_service.c
 * @brief Implementasi Layanan Terpadu Pilar E1: Akuisisi Data Metrologi & Akumulasi Energi
 */

#include "e1_metrology_service.h"
#include "Metrology.h"
#include "RegisterMap.h"
#include "SPI.h"
#include "Mock.h"
#include "main.h"

#if defined(EMBEDDED_HARDWARE_TARGET)
#include "secure_nsc.h"
#endif

static volatile uint32_t s_cf1_pulse_count = 0;
static volatile uint32_t s_cf2_pulse_count = 0;
static uint64_t          s_energy_exp_mj_accum = 0; /* Akumulator energi kirim/ekspor (+A) dalam Watt-ms (milliJoule) */
static uint64_t          s_energy_imp_mj_accum = 0; /* Akumulator energi terima/impor (-A) dalam Watt-ms (milliJoule) */
static ReactiveEnergyQuadrants_t s_reactive_quadrants = {0}; /* Akumulator energi reaktif 4-kuadran */

void e1_metrology_bringup_init(void)
{
    /* Sekuens Inisialisasi Register ADE9000 sesuai Desain Sistem Hal. 67-75 */
    /* 1. Buka kunci proteksi tulis register konfigurasi */
    ADE9000_SPI_TransactionWrite(ADE9000_WR_LOCK, ADE9000_SPI_DATA_16BIT, ADE9000_WR_UNLOCK_KEY);

    /* 2. Konfigurasi Gain PGA (1x pada seluruh 7 kanal ADC) & Mode Sensor CT */
    ADE9000_SPI_TransactionWrite(ADE9000_PGA_GAIN, ADE9000_SPI_DATA_16BIT, 0x0000U);
    ADE9000_SPI_TransactionWrite(ADE9000_CONFIG0,  ADE9000_SPI_DATA_32BIT, 0x00000000UL);

    /* 3. Route Zero-Crossing komposit ke pin CF3/ZX (PE8) */
    ADE9000_SPI_TransactionWrite(ADE9000_CONFIG1,  ADE9000_SPI_DATA_16BIT, ADE9000_CFG1_CF3_ZX);

    /* 4. Mode akumulasi 3-Fasa 4-Kawat Wye 50 Hz & CF1=Aktif, CF2=Reaktif, Lebar Pulsa 80ms */
    ADE9000_SPI_TransactionWrite(ADE9000_ACCMODE,  ADE9000_SPI_DATA_16BIT, 0x0100U);
    ADE9000_SPI_TransactionWrite(ADE9000_CFMODE,   ADE9000_SPI_DATA_16BIT, 0x0008U);
    ADE9000_SPI_TransactionWrite(ADE9000_CF_LCFG,  ADE9000_SPI_DATA_16BIT, 0x0050U);

    /* 5. Konstanta Pulsa Kalibrasi (1000 imp/kWh & 1000 imp/kVARh) dan Ambang No-Load (Anti-Creep) */
    ADE9000_SPI_TransactionWrite(ADE9000_WTHR,         ADE9000_SPI_DATA_32BIT, 0x00100000UL);
    ADE9000_SPI_TransactionWrite(ADE9000_VARTHR,       ADE9000_SPI_DATA_32BIT, 0x00100000UL);
    ADE9000_SPI_TransactionWrite(ADE9000_VATHR,        ADE9000_SPI_DATA_32BIT, 0x00100000UL);
    ADE9000_SPI_TransactionWrite(ADE9000_ACT_NL_LVL,   ADE9000_SPI_DATA_32BIT, 65536UL);
    ADE9000_SPI_TransactionWrite(ADE9000_REACT_NL_LVL, ADE9000_SPI_DATA_32BIT, 65536UL);
    ADE9000_SPI_TransactionWrite(ADE9000_APP_NL_LVL,   ADE9000_SPI_DATA_32BIT, 65536UL);

    /* 6. Ambang Deteksi Mutu Daya (Dip/Sag, Swell, Overcurrent) & Mask Interupsi IRQ0/IRQ1/EVENT */
    ADE9000_SPI_TransactionWrite(ADE9000_DIP_LVL,    ADE9000_SPI_DATA_32BIT, 11920000UL); /* ~160V */
    ADE9000_SPI_TransactionWrite(ADE9000_SWELL_LVL,  ADE9000_SPI_DATA_32BIT, 19740000UL); /* ~265V */
    ADE9000_SPI_TransactionWrite(ADE9000_OILVL,      ADE9000_SPI_DATA_32BIT, 49910000UL); /* ~100A */
    ADE9000_SPI_TransactionWrite(ADE9000_MASK0,      ADE9000_SPI_DATA_32BIT, 0x0003FFFFUL);
    ADE9000_SPI_TransactionWrite(ADE9000_MASK1,      ADE9000_SPI_DATA_32BIT, 0x0007FFFFUL);
    ADE9000_SPI_TransactionWrite(ADE9000_EVENT_MASK, ADE9000_SPI_DATA_32BIT, 0x0000003FUL);

    /* 7. Aktifkan Akumulator Energi Internal (8000 sampel = 1 detik) & Nyalakan DSP Engine */
    ADE9000_SPI_TransactionWrite(ADE9000_EGY_TIME, ADE9000_SPI_DATA_16BIT, 7999U);
    ADE9000_SPI_TransactionWrite(ADE9000_EP_CFG,   ADE9000_SPI_DATA_16BIT,
                                 ADE9000_EPCFG_EGY_PWR_EN | ADE9000_EPCFG_EGY_TMR_MODE | ADE9000_EPCFG_RD_RST_EN);
    ADE9000_SPI_TransactionWrite(ADE9000_RUN,      ADE9000_SPI_DATA_16BIT, 0x0001U);

    /* 8. Kunci kembali proteksi tulis register */
    ADE9000_SPI_TransactionWrite(ADE9000_WR_LOCK, ADE9000_SPI_DATA_16BIT, ADE9000_WR_LOCK_KEY);
}

void e1_metrology_calibrate_channel(uint8_t phase_idx, float ref_v, float ref_i, float ref_w)
{
    uint16_t vrms_reg = ADE9000_AVRMS, irms_reg = ADE9000_AIRMS, watt_reg = ADE9000_AWATT;
    uint16_t vgain_reg = ADE9000_AVGAIN, igain_reg = ADE9000_AIGAIN, pgain_reg = ADE9000_APGAIN;

    if (phase_idx == 1U)
    {
        vrms_reg = ADE9000_BVRMS; irms_reg = ADE9000_BIRMS; watt_reg = ADE9000_BWATT;
        vgain_reg = ADE9000_BVGAIN; igain_reg = ADE9000_BIGAIN; pgain_reg = ADE9000_BPGAIN;
    }
    else if (phase_idx == 2U)
    {
        vrms_reg = ADE9000_CVRMS; irms_reg = ADE9000_CIRMS; watt_reg = ADE9000_CWATT;
        vgain_reg = ADE9000_CVGAIN; igain_reg = ADE9000_CIGAIN; pgain_reg = ADE9000_CPGAIN;
    }

    uint32_t raw_v = 0, raw_i = 0, raw_w = 0;
    ADE9000_SPI_TransactionRead(vrms_reg, ADE9000_SPI_DATA_32BIT, &raw_v, NULL);
    ADE9000_SPI_TransactionRead(irms_reg, ADE9000_SPI_DATA_32BIT, &raw_i, NULL);
    ADE9000_SPI_TransactionRead(watt_reg, ADE9000_SPI_DATA_32BIT, &raw_w, NULL);

    float meas_v = Metrology_VoltageFromAVRMS(raw_v);
    float meas_i = Metrology_CurrentFromAIRMS(raw_i);
    float meas_w = Metrology_PowerFromAWATT((int32_t)raw_w);

    int32_t vgain = Metrology_CalculateGainRegister(ref_v, meas_v);
    int32_t igain = Metrology_CalculateGainRegister(ref_i, meas_i);
    int32_t pgain = Metrology_CalculateGainRegister(ref_w, meas_w);

    ADE9000_SPI_TransactionWrite(ADE9000_WR_LOCK, ADE9000_SPI_DATA_16BIT, ADE9000_WR_UNLOCK_KEY);
    ADE9000_SPI_TransactionWrite(vgain_reg, ADE9000_SPI_DATA_32BIT, (uint32_t)vgain);
    ADE9000_SPI_TransactionWrite(igain_reg, ADE9000_SPI_DATA_32BIT, (uint32_t)igain);
    ADE9000_SPI_TransactionWrite(pgain_reg, ADE9000_SPI_DATA_32BIT, (uint32_t)pgain);
    ADE9000_SPI_TransactionWrite(ADE9000_WR_LOCK, ADE9000_SPI_DATA_16BIT, ADE9000_WR_LOCK_KEY);
}

void e1_metrology_service_init(void)
{
    /* 1. Release Hardware Reset ADE9000 & Set Mode Run Normal (PSM0) */
    HAL_GPIO_WritePin(ADE_RESET_GPIO_Port, ADE_RESET_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(ADE_PM0_GPIO_Port, ADE_PM0_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(ADE_PM1_GPIO_Port, ADE_PM1_Pin, GPIO_PIN_RESET);

    /* 2. Inisialisasi SPI Transaksi & Mock Emulasi ADE9000 */
    ADE9000_Mock_Init();
    ADE9000_SPI_Init();

    /* 3. Set Baseline Nominal Metering: 220V, 15A, IN=0.05A, 9.9kW, 1.48kVAR, 10.01kVA, PF=0.990, 50.00Hz */
    ADE9000_Mock_SetAVRMS(16390389U);
    ADE9000_Mock_SetBVRMS(16390389U);
    ADE9000_Mock_SetCVRMS(16390389U);
    ADE9000_Mock_SetAIRMS(7486524U);
    ADE9000_Mock_SetBIRMS(7486524U);
    ADE9000_Mock_SetCIRMS(7486524U);
    ADE9000_Mock_SetNIRMS(24955U);       /* ~50 mA arus netral */
    ADE9000_Mock_SetAWATT(914203);       /* ~3300 W per fasa */
    ADE9000_Mock_SetBWATT(914203);
    ADE9000_Mock_SetCWATT(914203);
    ADE9000_Mock_SetAVAR(137130);        /* ~495 VAR per fasa */
    ADE9000_Mock_SetBVAR(137130);
    ADE9000_Mock_SetCVAR(137130);
    ADE9000_Mock_SetAVA(924432U);        /* ~3337 VA per fasa */
    ADE9000_Mock_SetBVA(924432U);
    ADE9000_Mock_SetCVA(924432U);
    ADE9000_Mock_SetAPF(132875550);      /* PF = 0.990 (Q1.27) */
    ADE9000_Mock_SetBPF(132875550);
    ADE9000_Mock_SetCPF(132875550);
    ADE9000_Mock_SetAPERIOD(10485759U);  /* 50.000 Hz */
    ADE9000_Mock_SetAWATTHR_HI(0x00000001);
    ADE9000_Mock_SetAWATTHR_LO(0x00001000);
    ADE9000_Mock_SetBWATTHR_HI(0x00000001);
    ADE9000_Mock_SetBWATTHR_LO(0x00001000);
    ADE9000_Mock_SetCWATTHR_HI(0x00000001);
    ADE9000_Mock_SetCWATTHR_LO(0x00001000);
    ADE9000_Mock_SetAVARHR_HI(0x00000001);
    ADE9000_Mock_SetAVARHR_LO(0x00001000);
    ADE9000_Mock_SetBVARHR_HI(0x00000001);
    ADE9000_Mock_SetBVARHR_LO(0x00001000);
    ADE9000_Mock_SetCVARHR_HI(0x00000001);
    ADE9000_Mock_SetCVARHR_LO(0x00001000);

    /* 4. Jalankan sekuens konfigurasi register internal ADE9000 */
    e1_metrology_bringup_init();
}

void e1_metrology_service_step(meter_measurements_t *meas, uint32_t delta_ms)
{
    if (meas == NULL) return;

    /* A. Baca seluruh 7 kanal RMS, Daya Aktif/Reaktif/Semu, PF, dan Periode dari ADE9000 */
    uint32_t raw_avrms = 0, raw_bvrms = 0, raw_cvrms = 0;
    uint32_t raw_airms = 0, raw_birms = 0, raw_cirms = 0, raw_nirms = 0;
    uint32_t raw_awatt = 0, raw_bwatt = 0, raw_cwatt = 0;
    uint32_t raw_avar  = 0, raw_bvar  = 0, raw_cvar  = 0;
    uint32_t raw_ava   = 0, raw_bva   = 0, raw_cva   = 0;
    uint32_t raw_apf   = 0, raw_aperiod = 0;

    ADE9000_SPI_TransactionRead(ADE9000_AVRMS,   ADE9000_SPI_DATA_32BIT, &raw_avrms,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_BVRMS,   ADE9000_SPI_DATA_32BIT, &raw_bvrms,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_CVRMS,   ADE9000_SPI_DATA_32BIT, &raw_cvrms,   NULL);

    ADE9000_SPI_TransactionRead(ADE9000_AIRMS,   ADE9000_SPI_DATA_32BIT, &raw_airms,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_BIRMS,   ADE9000_SPI_DATA_32BIT, &raw_birms,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_CIRMS,   ADE9000_SPI_DATA_32BIT, &raw_cirms,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_NIRMS,   ADE9000_SPI_DATA_32BIT, &raw_nirms,   NULL);

    ADE9000_SPI_TransactionRead(ADE9000_AWATT,   ADE9000_SPI_DATA_32BIT, &raw_awatt,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_BWATT,   ADE9000_SPI_DATA_32BIT, &raw_bwatt,   NULL);
    ADE9000_SPI_TransactionRead(ADE9000_CWATT,   ADE9000_SPI_DATA_32BIT, &raw_cwatt,   NULL);

    ADE9000_SPI_TransactionRead(ADE9000_AVAR,    ADE9000_SPI_DATA_32BIT, &raw_avar,    NULL);
    ADE9000_SPI_TransactionRead(ADE9000_BVAR,    ADE9000_SPI_DATA_32BIT, &raw_bvar,    NULL);
    ADE9000_SPI_TransactionRead(ADE9000_CVAR,    ADE9000_SPI_DATA_32BIT, &raw_cvar,    NULL);

    ADE9000_SPI_TransactionRead(ADE9000_AVA,     ADE9000_SPI_DATA_32BIT, &raw_ava,     NULL);
    ADE9000_SPI_TransactionRead(ADE9000_BVA,     ADE9000_SPI_DATA_32BIT, &raw_bva,     NULL);
    ADE9000_SPI_TransactionRead(ADE9000_CVA,     ADE9000_SPI_DATA_32BIT, &raw_cva,     NULL);

    ADE9000_SPI_TransactionRead(ADE9000_APF,     ADE9000_SPI_DATA_32BIT, &raw_apf,     NULL);
    ADE9000_SPI_TransactionRead(ADE9000_APERIOD, ADE9000_SPI_DATA_32BIT, &raw_aperiod, NULL);

    /* B. Konversi ke satuan fisik (Volt, Ampere, Watt, VAR, VA, PF, Hz) */
    float v_r = Metrology_VoltageFromAVRMS(raw_avrms);
    float v_s = Metrology_VoltageFromBVRMS(raw_bvrms);
    float v_t = Metrology_VoltageFromCVRMS(raw_cvrms);

    float i_r = Metrology_CurrentFromAIRMS(raw_airms);
    float i_s = Metrology_CurrentFromBIRMS(raw_birms);
    float i_t = Metrology_CurrentFromCIRMS(raw_cirms);
    float i_n = Metrology_CurrentFromNIRMS(raw_nirms);

    float p_r = Metrology_PowerFromAWATT((int32_t)raw_awatt);
    float p_s = Metrology_PowerFromBWATT((int32_t)raw_bwatt);
    float p_t = Metrology_PowerFromCWATT((int32_t)raw_cwatt);

    float q_r = Metrology_ReactivePowerFromAVAR((int32_t)raw_avar);
    float q_s = Metrology_ReactivePowerFromBVAR((int32_t)raw_bvar);
    float q_t = Metrology_ReactivePowerFromCVAR((int32_t)raw_cvar);

    float s_r = Metrology_ApparentPowerFromAVA(raw_ava);
    float s_s = Metrology_ApparentPowerFromBVA(raw_bva);
    float s_t = Metrology_ApparentPowerFromCVA(raw_cva);

    float freq_hz = Metrology_FrequencyFromPeriod(raw_aperiod);

    /* C. Simpan ke struct pengukuran standar (skala dVolts, mAmps, Watt, VAR, VA, permille, mHz) */
    meas->voltage_r_dvolts = (v_r > 0.0f) ? (uint32_t)(v_r * 10.0f) : 0U;
    meas->voltage_s_dvolts = (v_s > 0.0f) ? (uint32_t)(v_s * 10.0f) : 0U;
    meas->voltage_t_dvolts = (v_t > 0.0f) ? (uint32_t)(v_t * 10.0f) : 0U;

    meas->current_r_mamps = (i_r > 0.0f) ? (uint32_t)(i_r * 1000.0f) : 0U;
    meas->current_s_mamps = (i_s > 0.0f) ? (uint32_t)(i_s * 1000.0f) : 0U;
    meas->current_t_mamps = (i_t > 0.0f) ? (uint32_t)(i_t * 1000.0f) : 0U;
    meas->current_n_mamps = (i_n > 0.0f) ? (uint32_t)(i_n * 1000.0f) : 0U;

    meas->active_power_w     = (int32_t)(p_r + p_s + p_t);
    meas->reactive_power_var = (int32_t)(q_r + q_s + q_t);
    meas->active_power_imp_w = (meas->active_power_w < 0) ? -meas->active_power_w : 0;
    meas->reactive_power_imp_var = (meas->reactive_power_var < 0) ? -meas->reactive_power_var : 0;
    float total_s            = s_r + s_s + s_t;
    meas->apparent_power_va  = (total_s > 0.0f) ? (uint32_t)total_s : 0U;
    meas->power_factor_ppm   = Metrology_PowerFactorPermilleFromRaw((int32_t)raw_apf);
    meas->frequency_mhz      = (freq_hz > 0.0f) ? (uint16_t)(freq_hz * 1000.0f) : 50000U;

    /* Sudut Fasa Tegangan & Arus (Default nominal 3-fasa R-S-T seimbang: UR=0, US=120, UT=240) */
    uint32_t raw_angl_ab = 0, raw_angl_ac = 0;
    uint32_t raw_angl_va_ia = 0, raw_angl_vb_ib = 0, raw_angl_vc_ic = 0;
    ADE9000_SPI_TransactionRead(ADE9000_ANGL_VA_VB, ADE9000_SPI_DATA_16BIT, &raw_angl_ab, NULL);
    ADE9000_SPI_TransactionRead(ADE9000_ANGL_VA_VC, ADE9000_SPI_DATA_16BIT, &raw_angl_ac, NULL);
    ADE9000_SPI_TransactionRead(ADE9000_ANGL_VA_IA, ADE9000_SPI_DATA_16BIT, &raw_angl_va_ia, NULL);
    ADE9000_SPI_TransactionRead(ADE9000_ANGL_VB_IB, ADE9000_SPI_DATA_16BIT, &raw_angl_vb_ib, NULL);
    ADE9000_SPI_TransactionRead(ADE9000_ANGL_VC_IC, ADE9000_SPI_DATA_16BIT, &raw_angl_vc_ic, NULL);

    meas->angle_ur_deg = 0U;
    meas->angle_us_deg = (raw_angl_ab > 0U) ? (uint16_t)(((uint32_t)raw_angl_ab * 360U) / 2048U) : 120U;
    meas->angle_ut_deg = (raw_angl_ac > 0U) ? (uint16_t)(((uint32_t)raw_angl_ac * 360U) / 2048U) : 240U;
    meas->angle_ir_deg = (raw_angl_va_ia > 0U) ? (uint16_t)(((uint32_t)raw_angl_va_ia * 360U) / 2048U) : 8U;
    meas->angle_is_deg = (raw_angl_vb_ib > 0U) ? (uint16_t)(((uint32_t)raw_angl_vb_ib * 360U) / 2048U) : 8U;
    meas->angle_it_deg = (raw_angl_vc_ic > 0U) ? (uint16_t)(((uint32_t)raw_angl_vc_ic * 360U) / 2048U) : 8U;

    /* D. Akumulasi energi aktif presisi tinggi (3.600.000 W*ms = 1 Wh) dengan anti-creep SPLN D3.006 */
    if (meas->active_power_w >= 3) /* Ambang anti-creep daya kirim (+A) */
    {
        s_energy_exp_mj_accum += ((uint64_t)meas->active_power_w * (uint64_t)delta_ms);
        while (s_energy_exp_mj_accum >= 3600000ULL)
        {
            meas->active_energy_wh++;
            s_energy_exp_mj_accum -= 3600000ULL;
        }
    }
    else if (meas->active_power_w <= -3) /* Ambang anti-creep daya terima/impor (-A) */
    {
        s_energy_imp_mj_accum += ((uint64_t)(-meas->active_power_w) * (uint64_t)delta_ms);
        while (s_energy_imp_mj_accum >= 3600000ULL)
        {
            meas->active_energy_import_wh++;
            s_energy_imp_mj_accum -= 3600000ULL;
        }
    }

    /* E. Akumulasi Energi Reaktif 4-Kuadran (SPLN D3.006:2021 / ADE9000_Metrology_2) */
    double delta_sec = (double)delta_ms / 1000.0;
    Metrology_AccumulateReactiveQuadrant(&s_reactive_quadrants, (double)p_r, (double)q_r, delta_sec);
    Metrology_AccumulateReactiveQuadrant(&s_reactive_quadrants, (double)p_s, (double)q_s, delta_sec);
    Metrology_AccumulateReactiveQuadrant(&s_reactive_quadrants, (double)p_t, (double)q_t, delta_sec);

    meas->reactive_energy_exp_varh = (uint64_t)(s_reactive_quadrants.q1_varh + s_reactive_quadrants.q2_varh);
    meas->reactive_energy_imp_varh = (uint64_t)(s_reactive_quadrants.q3_varh + s_reactive_quadrants.q4_varh);
    meas->reactive_energy_varh = meas->reactive_energy_exp_varh + meas->reactive_energy_imp_varh;
}

const ReactiveEnergyQuadrants_t *e1_metrology_get_reactive_quadrants(void)
{
    return &s_reactive_quadrants;
}

void e1_metrology_set_power_mode(bool sleep)
{
    if (sleep)
    {
        /* Mode Hemat Daya Ekstrem saat Padam (PSM3: PM0=1, PM1=1) */
        HAL_GPIO_WritePin(ADE_PM0_GPIO_Port, ADE_PM0_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(ADE_PM1_GPIO_Port, ADE_PM1_Pin, GPIO_PIN_SET);
    }
    else
    {
        /* Mode Operasional Penuh (PSM0: PM0=0, PM1=0) */
        HAL_GPIO_WritePin(ADE_PM0_GPIO_Port, ADE_PM0_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ADE_PM1_GPIO_Port, ADE_PM1_Pin, GPIO_PIN_RESET);
    }
}

void e1_metrology_on_active_pulse(void)
{
    s_cf1_pulse_count++;
#if defined(EMBEDDED_HARDWARE_TARGET)
    SECURE_RecordPulseCF1();
#endif
}

void e1_metrology_on_reactive_pulse(void)
{
    s_cf2_pulse_count++;
#if defined(EMBEDDED_HARDWARE_TARGET)
    SECURE_RecordPulseCF2();
#endif
}

uint32_t e1_metrology_get_active_pulses(void)
{
    return s_cf1_pulse_count;
}

uint32_t e1_metrology_get_reactive_pulses(void)
{
    return s_cf2_pulse_count;
}
