#ifndef ADE9000_REGISTER_MAP_H
#define ADE9000_REGISTER_MAP_H

#include <stdint.h>


/* =========================================================
 * PHASE A CALIBRATION
 * ========================================================= */

#define ADE9000_AIGAIN          0x000   // Phase A current gain calibration
#define ADE9000_AIGAIN0         0x001   // Phase A current gain calibration - point 0
#define ADE9000_AIGAIN1         0x002   
#define ADE9000_AIGAIN2         0x003   
#define ADE9000_AIGAIN3         0x004   
#define ADE9000_AIGAIN4         0x005   

#define ADE9000_APHCAL0         0x006   // Phase A current phase calibration - point 0
#define ADE9000_APHCAL1         0x007   
#define ADE9000_APHCAL2         0x008   
#define ADE9000_APHCAL3         0x009   
#define ADE9000_APHCAL4         0x00A   
#define ADE9000_PGA_GAIN        0x00A   // PGA Gain configuration (Desain Sistem Hal. 67)

#define ADE9000_AVGAIN          0x00B   // Phase A voltage gain calibration
#define ADE9000_AIRMSOS         0x00C   // Kalibrasi offset RMS arus fase A
#define ADE9000_AVRMSOS         0x00D   // Kalibrasi offset RMS tegangan fase A
#define ADE9000_APGAIN          0x00E   // Kalibrasi gain daya fase A
#define ADE9000_AWATTOS         0x00F   // Kalibrasi offset daya aktif fase A
#define ADE9000_AVAROS          0x010   // Kalibrasi offset daya reaktif fase A
#define ADE9000_AFWATTOS        0x011   // Kalibrasi offset daya aktif fundamental fase A
#define ADE9000_AFVAROS         0x012   // Kalibrasi offset daya reaktif fundamental fase A
#define ADE9000_AIFRMSOS        0x013   // Kalibrasi offset RMS arus fundamental fase A
#define ADE9000_AVFRMSOS        0x014   // Kalibrasi offset RMS tegangan fundamental fase A
#define ADE9000_AVRMSONEOS      0x015   // Kalibrasi offset fast RMS tegangan fase A
#define ADE9000_AIRMSONEOS      0x016   // Kalibrasi offset fast RMS arus fase A
#define ADE9000_AVRMS1012OS     0x017   // Kalibrasi offset RMS tegangan 10/12-cycle fase A
#define ADE9000_AIRMS1012OS     0x018   // Kalibrasi offset RMS arus 10/12-cycle fase A

#define ADE9000_AWATT_ACC_LO    0x2E4   // Phase A accumulated total active power, LSB
#define ADE9000_AWATT_ACC_HI    0x2E5   // Phase A accumulated total active power, MSB
#define ADE9000_AWATTHR_LO      0x2E6   // Phase A accumulated total active energy, LSB
#define ADE9000_AWATTHR_HI      0x2E7   // Phase A accumulated total active energy, MSB
#define ADE9000_AVARHR_LO       0x2F0   // Phase A reactive energy LSB (ADE9000_Metrology_2)
#define ADE9000_AVARHR_HI       0x2F1   // Phase A reactive energy MSB (ADE9000_Metrology_2)
#define ADE9000_AVAHR_LO        0x2EE   // Phase A accumulated total apparent energy, LSB
#define ADE9000_AVAHR_HI        0x2EF   // Phase A accumulated total apparent energy, MSB

/* =========================================================
 * PHASE B CALIBRATION
 * ========================================================= */

#define ADE9000_BIGAIN          0x020   // Kalibrasi gain arus fase B
#define ADE9000_BIGAIN0         0x021   // Kalibrasi gain arus fase B - titik 0
#define ADE9000_BIGAIN1         0x022   
#define ADE9000_BIGAIN2         0x023  
#define ADE9000_BIGAIN3         0x024   
#define ADE9000_BIGAIN4         0x025   

#define ADE9000_BPHCAL0         0x026   // Kalibrasi fase arus B - titik 0
#define ADE9000_BPHCAL1         0x027   
#define ADE9000_BPHCAL2         0x028   
#define ADE9000_BPHCAL3         0x029   
#define ADE9000_BPHCAL4         0x02A   

#define ADE9000_BVGAIN          0x02B   // Kalibrasi gain tegangan fase B
#define ADE9000_BIRMSOS         0x02C   // Kalibrasi offset RMS arus fase B
#define ADE9000_BVRMSOS         0x02D   // Kalibrasi offset RMS tegangan fase B
#define ADE9000_BPGAIN          0x02E   // Kalibrasi gain daya fase B
#define ADE9000_BWATTOS         0x02F   // Kalibrasi offset daya aktif fase B
#define ADE9000_BVAROS          0x030   // Kalibrasi offset daya reaktif fase B
#define ADE9000_BFWATTOS        0x031   // Kalibrasi offset daya aktif fundamental fase B
#define ADE9000_BFVAROS         0x032   // Kalibrasi offset daya reaktif fundamental fase B
#define ADE9000_BIFRMSOS        0x033   // Kalibrasi offset RMS arus fundamental fase B
#define ADE9000_BVFRMSOS        0x034   // Kalibrasi offset RMS tegangan fundamental fase B
#define ADE9000_BVRMSONEOS      0x035   // Kalibrasi offset fast RMS tegangan fase B
#define ADE9000_BIRMSONEOS      0x036   // Kalibrasi offset fast RMS arus fase B
#define ADE9000_BVRMS1012OS     0x037   // Kalibrasi offset RMS tegangan 10/12-cycle fase B
#define ADE9000_BIRMS1012OS     0x038   // Kalibrasi offset RMS arus 10/12-cycle fase B

#define ADE9000_BWATT_ACC_LO    0x320   // Phase B accumulated total active power, LSB
#define ADE9000_BWATT_ACC_HI    0x321   // Phase B accumulated total active power, MSB
#define ADE9000_BWATTHR_LO      0x322   // Phase B accumulated total active energy, LSB
#define ADE9000_BWATTHR_HI      0x323   // Phase B accumulated total active energy, MSB
#define ADE9000_BVARHR_LO       0x324   // Phase B reactive energy LSB (ADE9000_Metrology_2)
#define ADE9000_BVARHR_HI       0x325   // Phase B reactive energy MSB (ADE9000_Metrology_2)
#define ADE9000_BVAHR_LO        0x32A   // Phase B accumulated total apparent energy, LSB
#define ADE9000_BVAHR_HI        0x32B   // Phase B accumulated total apparent energy, MSB

/* =========================================================
 * PHASE C CALIBRATION
 * ========================================================= */

#define ADE9000_CIGAIN          0x040   // Kalibrasi gain arus fase C
#define ADE9000_CIGAIN0         0x041   // Kalibrasi gain arus fase C - titik 0
#define ADE9000_CIGAIN1         0x042   
#define ADE9000_CIGAIN2         0x043   
#define ADE9000_CIGAIN3         0x044   
#define ADE9000_CIGAIN4         0x045   

#define ADE9000_CPHCAL0         0x046   // Kalibrasi fase arus C - titik 0
#define ADE9000_CPHCAL1         0x047   
#define ADE9000_CPHCAL2         0x048   
#define ADE9000_CPHCAL3         0x049   
#define ADE9000_CPHCAL4         0x04A   

#define ADE9000_CVGAIN          0x04B   // Kalibrasi gain tegangan fase C
#define ADE9000_CIRMSOS         0x04C   // Kalibrasi offset RMS arus fase C
#define ADE9000_CVRMSOS         0x04D   // Kalibrasi offset RMS tegangan fase C
#define ADE9000_CPGAIN          0x04E   // Kalibrasi gain daya fase C
#define ADE9000_CWATTOS         0x04F   // Kalibrasi offset daya aktif fase C
#define ADE9000_CVAROS          0x050   // Kalibrasi offset daya reaktif fase C
#define ADE9000_CFWATTOS        0x051   // Kalibrasi offset daya aktif fundamental fase C
#define ADE9000_CFVAROS         0x052   // Kalibrasi offset daya reaktif fundamental fase C
#define ADE9000_CIFRMSOS        0x053   // Kalibrasi offset RMS arus fundamental fase C
#define ADE9000_CVFRMSOS        0x054   // Kalibrasi offset RMS tegangan fundamental fase C
#define ADE9000_CVRMSONEOS      0x055   // Kalibrasi offset fast RMS tegangan fase C
#define ADE9000_CIRMSONEOS      0x056   // Kalibrasi offset fast RMS arus fase C
#define ADE9000_CVRMS1012OS     0x057   // Kalibrasi offset RMS tegangan 10/12-cycle fase C
#define ADE9000_CIRMS1012OS     0x058   // Kalibrasi offset RMS arus 10/12-cycle fase C

#define ADE9000_CWATT_ACC_LO    0x35C   // Phase C accumulated total active power, LSB
#define ADE9000_CWATT_ACC_HI    0x35D   // Phase C accumulated total active power, MSB
#define ADE9000_CWATTHR_LO      0x35E   // Phase C accumulated total active energy, LSB
#define ADE9000_CWATTHR_HI      0x35F   // Phase C accumulated total active energy, MSB
#define ADE9000_CVARHR_LO       0x368   // Phase C reactive energy LSB (ADE9000_Metrology_2)
#define ADE9000_CVARHR_HI       0x369   // Phase C reactive energy MSB (ADE9000_Metrology_2)
#define ADE9000_CVAHR_LO        0x366   // Phase C accumulated total apparent energy, LSB
#define ADE9000_CVAHR_HI        0x367   // Phase C accumulated total apparent energy, MSB

/* =========================================================
 * MULTIPOINT CALIBRATION
 * ========================================================= */

#define ADE9000_CONFIG0         0x060   // Konfigurasi multipoint calibration

#define ADE9000_MTTHR_L0        0x061   // Batas bawah multipoint calibration titik 0
#define ADE9000_MTTHR_L1        0x062   
#define ADE9000_MTTHR_L2        0x063   
#define ADE9000_MTTHR_L3        0x064   
#define ADE9000_MTTHR_L4        0x065   

#define ADE9000_MTTHR_H0        0x066   // Batas atas multipoint calibration titik 0
#define ADE9000_MTTHR_H1        0x067   
#define ADE9000_MTTHR_H2        0x068   
#define ADE9000_MTTHR_H3        0x069   
#define ADE9000_MTTHR_H4        0x06A   


/* =========================================================
 * NEUTRAL CALIBRATION
 * ========================================================= */

#define ADE9000_NIRMSOS         0x06B   // Kalibrasi offset RMS arus neutral
#define ADE9000_ISUMRMSOS       0x06C   // Kalibrasi offset RMS arus hasil penjumlahan
#define ADE9000_NIGAIN          0x06D   // Kalibrasi gain arus neutral
#define ADE9000_NPHCAL          0x06E   // Kalibrasi fase arus neutral
#define ADE9000_NIRMSONEOS      0x06F   // Kalibrasi offset fast RMS arus neutral
#define ADE9000_NIRMS1012OS     0x070   // Kalibrasi offset RMS arus 10/12-cycle neutral

#define ADE9000_VNOM            0x071   // Nilai nominal tegangan fase untuk perhitungan VA
#define ADE9000_DICOEFF         0x072   // Koefisien digital integrator
#define ADE9000_ISUMLVL         0x073   // Threshold untuk deteksi ketidaksesuaian ISUMRMS
#define ADE9000_NIRMS           0x266   // RMS arus neutral


/* =========================================================
 * PHASE A METROLOGY
 * ========================================================= */

#define ADE9000_AIRMS           0x20C   // RMS arus total fase A
#define ADE9000_AVRMS           0x20D   // RMS tegangan total fase A
#define ADE9000_AIFRMS          0x20E   // RMS arus fundamental fase A
#define ADE9000_AVFRMS          0x20F   // RMS tegangan fundamental fase A

#define ADE9000_AWATT           0x210   // Daya aktif total fase A
#define ADE9000_AVAR            0x211   // Daya reaktif total fase A
#define ADE9000_AVA             0x212   // Daya semu total fase A
#define ADE9000_AFWATT          0x213   // Daya aktif fundamental fase A
#define ADE9000_AFVAR           0x214   // Daya reaktif fundamental fase A
#define ADE9000_AFVA            0x215   // Daya semu fundamental fase A
#define ADE9000_APF             0x216   // Power factor fase A


/* =========================================================
 * PHASE B METROLOGY
 * ========================================================= */

#define ADE9000_BIRMS           0x22C   // RMS arus total fase B
#define ADE9000_BVRMS           0x22D   // RMS tegangan total fase B
#define ADE9000_BIFRMS          0x22E   // RMS arus fundamental fase B
#define ADE9000_BVFRMS          0x22F   // RMS tegangan fundamental fase B

#define ADE9000_BWATT           0x230   // Daya aktif total fase B
#define ADE9000_BVAR            0x231   // Daya reaktif total fase B
#define ADE9000_BVA             0x232   // Daya semu total fase B
#define ADE9000_BFWATT          0x233   // Daya aktif fundamental fase B
#define ADE9000_BFVAR           0x234   // Daya reaktif fundamental fase B
#define ADE9000_BFVA            0x235   // Daya semu fundamental fase B
#define ADE9000_BPF             0x236   // Power factor fase B


/* =========================================================
 * PHASE C METROLOGY
 * ========================================================= */

#define ADE9000_CIRMS           0x24C   // RMS arus total fase C
#define ADE9000_CVRMS           0x24D   // RMS tegangan total fase C
#define ADE9000_CIFRMS          0x24E   // RMS arus fundamental fase C
#define ADE9000_CVFRMS          0x24F   // RMS tegangan fundamental fase C

#define ADE9000_CWATT           0x250   // Daya aktif total fase C
#define ADE9000_CVAR            0x251   // Daya reaktif total fase C
#define ADE9000_CVA             0x252   // Daya semu total fase C
#define ADE9000_CFWATT          0x253   // Daya aktif fundamental fase C
#define ADE9000_CFVAR           0x254   // Daya reaktif fundamental fase C
#define ADE9000_CFVA            0x255   // Daya semu fundamental fase C
#define ADE9000_CPF             0x256   // Power factor fase C


/* =========================================================
 * ADDITIONAL RMS / THD METROLOGY
 * ========================================================= */

#define ADE9000_AVTHD           0x217   // THD tegangan fase A
#define ADE9000_AITHD           0x218   // THD arus fase A

#define ADE9000_BVTHD           0x237   // THD tegangan fase B
#define ADE9000_BITHD           0x238   // THD arus fase B

#define ADE9000_CVTHD           0x257   // THD tegangan fase C
#define ADE9000_CITHD           0x258   // THD arus fase C

#define ADE9000_AIRMSONE        0x219   // Fast RMS arus fase A
#define ADE9000_AVRMSONE        0x21A   // Fast RMS tegangan fase A
#define ADE9000_AIRMS1012       0x21B   // RMS arus 10/12-cycle fase A
#define ADE9000_AVRMS1012       0x21C   // RMS tegangan 10/12-cycle fase A

#define ADE9000_BIRMSONE        0x239   // Fast RMS arus fase B
#define ADE9000_BVRMSONE        0x23A   // Fast RMS tegangan fase B
#define ADE9000_BIRMS1012       0x23B   // RMS arus 10/12-cycle fase B
#define ADE9000_BVRMS1012       0x23C   // RMS tegangan 10/12-cycle fase B

#define ADE9000_CIRMSONE        0x259   // Fast RMS arus fase C
#define ADE9000_CVRMSONE        0x25A   // Fast RMS tegangan fase C
#define ADE9000_CIRMS1012       0x25B   // RMS arus 10/12-cycle fase C
#define ADE9000_CVRMS1012       0x25C   // RMS tegangan 10/12-cycle fase C


/* =========================================================
 * PEAK / STATUS / EVENT
 * ========================================================= */

#define ADE9000_IPEAK           0x400   // Nilai peak
#define ADE9000_VPEAK           0x401  

#define ADE9000_STATUS0         0x402   // Status register 0
#define ADE9000_STATUS1         0x403   
#define ADE9000_EVENT_STATUS    0x404   // Status event

#define ADE9000_MASK0           0x405   // Enable interrupt register 0
#define ADE9000_MASK1           0x406   
#define ADE9000_EVENT_MASK      0x407   // Enable event register


/* =========================================================
 * OVERCURRENT DETECTION
 * ========================================================= */

#define ADE9000_OILVL           0x409   // Threshold deteksi overcurrent
#define ADE9000_OIA             0x40A   // Nilai RMS overcurrent fase A
#define ADE9000_OIB             0x40B   
#define ADE9000_OIC             0x40C   
#define ADE9000_OIN             0x40D   // Nilai RMS overcurrent neutral


/* =========================================================
 * DIP / SWELL
 * ========================================================= */

#define ADE9000_DIP_LVL         0x410   // Threshold deteksi voltage dip
#define ADE9000_DIPA            0x411   // RMS tegangan fase A saat dip
#define ADE9000_DIPB            0x412   
#define ADE9000_DIPC            0x413   

#define ADE9000_SWELL_LVL       0x414   // Threshold deteksi voltage swell
#define ADE9000_SWELLA           0x415   // RMS tegangan fase A saat swell
#define ADE9000_SWELLB           0x416   
#define ADE9000_SWELLC           0x417   


/* =========================================================
 * PERIOD / FREQUENCY
 * ========================================================= */

#define ADE9000_APERIOD         0x418   // Periode tegangan fase A
#define ADE9000_BPERIOD         0x419   
#define ADE9000_CPERIOD         0x41A   
#define ADE9000_COM_PERIOD      0x41B   // Periode gabungan tegangan A, B, dan C


/* =========================================================
 * NO LOAD / CF CALIBRATION OUTPUT
 * ========================================================= */

#define ADE9000_ACT_NL_LVL      0x41C   // Threshold no-load daya aktif
#define ADE9000_REACT_NL_LVL    0x41D   // Threshold no-load daya reaktif
#define ADE9000_APP_NL_LVL      0x41E   // Threshold no-load daya semu
#define ADE9000_PHNOLOAD        0x41F   // Status no-load fase

#define ADE9000_WTHR            0x420   // Threshold output CF daya aktif
#define ADE9000_VARTHR          0x421   // Threshold output CF daya reaktif
#define ADE9000_VATHR           0x422   // Threshold output CF daya semu
#define ADE9000_LAST_DATA_32    0x423   // Data terakhir transaksi SPI 32-bit
#define ADE9000_ADC_REDIRECT    0x424   // Redirect output ADC ke datapath
#define ADE9000_CF_LCFG         0x425   // Konfigurasi lebar pulsa CFx


/* =========================================================
 * DEVICE IDENTIFICATION
 * ========================================================= */

#define ADE9000_PART_ID         0x472   // Identifikasi IC ADE9000 (Bit 20 = 1)
#define ADE9000_TEMP_TRIM       0x474   // Kalibrasi gain dan offset sensor suhu


/* =========================================================
 * CONTROL & ENERGY ACCUMULATION (Desain Sistem Hal. 67-71)
 * ========================================================= */

#define ADE9000_RUN             0x480   // Mengaktifkan / menjalankan DSP pengukuran (16-bit)
#define ADE9000_CONFIG1         0x481   // Configuration Register 1 (16-bit)

#define ADE9000_ANGL_VA_VB      0x482   // Selisih waktu zero-crossing tegangan A dan B
#define ADE9000_ANGL_VB_VC      0x483   // Selisih waktu zero-crossing tegangan B dan C
#define ADE9000_ANGL_VA_VC      0x484   // Selisih waktu zero-crossing tegangan A dan C
#define ADE9000_ANGL_VA_IA      0x485   // Sudut fase tegangan A ke arus A
#define ADE9000_ANGL_VB_IB      0x486   // Sudut fase tegangan B ke arus B
#define ADE9000_ANGL_VC_IC      0x487   // Sudut fase tegangan C ke arus C

#define ADE9000_COMPMODE        0x48E   // Computation mode register
#define ADE9000_CFMODE          0x490   // CF output configuration register (Desain Sistem Hal. 69)
#define ADE9000_ACCMODE         0x492   // Accumulation mode & 50/60Hz SELFREQ (Desain Sistem Hal. 67)
#define ADE9000_CONFIG3         0x493   // Overcurrent & Peak configuration

#define ADE9000_CRC_RSLT        0x4A8   // CRC of configuration registers
#define ADE9000_CRC_FORCE       0x4A9   // Force configuration CRC update
#define ADE9000_WR_LOCK         0x4AF   // Configuration write protection (0x3C64 lock / 0x4AD1 unlock)
#define ADE9000_EP_CFG          0x4B0   // Energy and power accumulation configuration (Hal. 68, 71)
#define ADE9000_EGY_TIME        0x4B2   // Energy accumulation update time (Hal. 68)

/* =========================================================
 * KEY BIT DEFINITIONS (Desain Sistem Hal. 67-71)
 * ========================================================= */

#define ADE9000_PART_ID_MASK        (1UL << 20) /* Bit 20: ADE9000_ID */
#define ADE9000_WR_LOCK_KEY         0x3C64U     /* Kunci proteksi konfigurasi */
#define ADE9000_WR_UNLOCK_KEY       0x4AD1U     /* Buka kunci konfigurasi */

/* CONFIG1 (0x481) Bits */
#define ADE9000_CFG1_EXT_REF        (1U << 15)
#define ADE9000_CFG1_IRQ0_ON_IRQ1   (1U << 12)
#define ADE9000_CFG1_BURST_EN       (1U << 11)
#define ADE9000_CFG1_DIP_SWELL_MODE (1U << 10)
#define ADE9000_CFG1_CF_ACC_CLR     (1U << 5)
#define ADE9000_CFG1_CF3_ZX         (1U << 1)

/* EP_CFG (0x4B0) Bits */
#define ADE9000_EPCFG_EGY_PWR_EN    (1U << 0)
#define ADE9000_EPCFG_EGY_TMR_MODE  (1U << 1)
#define ADE9000_EPCFG_EGY_LD_ACCUM  (1U << 4)
#define ADE9000_EPCFG_RD_RST_EN     (1U << 5)

/* STATUS0 / MASK0 (0x402 / 0x405) Bits */
#define ADE9000_ST0_EGYRDY          (1UL << 0)
#define ADE9000_ST0_PWRRDY          (1UL << 16)
#define ADE9000_ST0_RMSONERDY       (1UL << 17)
#define ADE9000_ST0_RMS1012RDY      (1UL << 18)
#define ADE9000_ST0_THD_PF_RDY      (1UL << 21)

/* STATUS1 / MASK1 (0x403 / 0x406) Bits */
#define ADE9000_ST1_NOLOAD          (1UL << 0)
#define ADE9000_ST1_REVAPA          (1UL << 6)
#define ADE9000_ST1_REVAPB          (1UL << 7)
#define ADE9000_ST1_REVAPC          (1UL << 8)
#define ADE9000_ST1_OI              (1UL << 17)
#define ADE9000_ST1_SEQERR          (1UL << 19)
#define ADE9000_ST1_DIPA            (1UL << 20)
#define ADE9000_ST1_DIPB            (1UL << 21)
#define ADE9000_ST1_DIPC            (1UL << 22)
#define ADE9000_ST1_SWELLA          (1UL << 23)
#define ADE9000_ST1_SWELLB          (1UL << 24)
#define ADE9000_ST1_SWELLC          (1UL << 25)

#endif /* ADE9000_REGISTER_MAP_H */
