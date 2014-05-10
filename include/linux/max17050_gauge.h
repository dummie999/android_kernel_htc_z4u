#ifndef __MAX17050_GAUGE_H__
#define __MAX17050_GAUGE_H__

#define MAX17050_I2C_BUS_ID   	1
#define MAX17050_I2C_SLAVE_ADDR 0x36
#define MAX17050_I2C_RETRY_TIMES 10
#define TRUE 1
#define FALSE 0
#define MAXIM_I2C_DEBUG 0
#define MAXIM_BATTERY_FG_LOG 1
#define MAXIM_BATTERY_FG_LOG_DEBUG 1
#define MAXIM_BATTERY_DEBUG 0

#define MAX17050_STATUS_PORF  (1 << 6)	
#define MAX17050_STATUS_SMOD  (1 << 5)	
#define MAX17050_STATUS_NBEN  (1 << 4)	
#define MAX17050_STATUS_AIN0  (1 << 0)
#define MAX17050_STATUS_AIN1  (1 << 1)


struct battery_type{

		BOOL is_power_on_reset;

		u32 voltage_mV;
		s32 current_mA;
		INT32 discharge_mA;
		INT32 charge_counter_mAh;
		s32 temp_01c;
		INT32 last_temp_01c;
		INT32 id_ohm;
		INT32 vref_mv;

		u32 voltage_adc;
		s16 current_adc;
		INT32 discharge_adc;
		INT32 charge_counter_adc;
		s16 temp_adc;
		INT32 last_temp_adc;
		INT32 id_adc;
		INT32 vref_adc;

		INT32 id_index;
		u32 charge_full_design_mAh;
		u32 charge_full_real_mAh;

		INT32 temp_index;
		INT32 temp_check_index;

		INT32 KADC_01p;
		INT32 RARC_01p;
		INT32 pd_m;

		INT32 software_charge_counter_mAms;
		INT32 thermal_id;
		u32 capacity_raw;
		u32 capacity_raw_hex;
		INT32 charge_full_capacity_mAh;
};

struct protect_flags_type{

		BOOL is_charging_enable_available;
		BOOL is_charging_high_current_avaialble;
		BOOL is_charging_indicator_available;
		BOOL is_charging_reverse_protect; 
		BOOL is_battery_dead;
		BOOL is_temperature_fault;
#if 0
		BOOL is_battery_overtemp;
#endif
		BOOL is_fake_room_temp;
		int (*func_update_charging_protect_flag)(int, int, int, BOOL*, BOOL*, BOOL*);
};



struct max17050_fg {
	struct device *dev;
#if MAXIM_BATTERY_FG_LOG
	struct wake_lock fg_log_wake_lock;
	struct delayed_work fg_log_work;
#endif
};


enum max17050_fg_register {
	MAX17050_FG_STATUS   	= 0x00,
	MAX17050_FG_VALRT_Th	= 0x01,
	MAX17050_FG_TALRT_Th	= 0x02,
	MAX17050_FG_SALRT_Th	= 0x03,
	MAX17050_FG_AtRate		= 0x04,
	MAX17050_FG_RepCap		= 0x05,
	MAX17050_FG_RepSOC		= 0x06,
	MAX17050_FG_Age			= 0x07,
	MAX17050_FG_TEMP		= 0x08,
	MAX17050_FG_VCELL		= 0x09,
	MAX17050_FG_Current		= 0x0A,
	MAX17050_FG_AvgCurrent	= 0x0B,
	MAX17050_FG_Qresidual	= 0x0C,
	MAX17050_FG_SOC			= 0x0D,
	MAX17050_FG_AvSOC		= 0x0E,
	MAX17050_FG_RemCap		= 0x0F,
	MAX17050_FG_FullCAP		= 0x10,
	MAX17050_FG_TTE			= 0x11,
	MAX17050_FG_QRtable00	= 0x12,
	MAX17050_FG_FullSOCthr	= 0x13,
	MAX17050_FG_RSLOW		= 0x14,
	MAX17050_FG_RFAST		= 0x15,
	MAX17050_FG_AvgTA		= 0x16,
	MAX17050_FG_Cycles		= 0x17,
	MAX17050_FG_DesignCap	= 0x18,
	MAX17050_FG_AvgVCELL	= 0x19,
	MAX17050_FG_MinMaxTemp	= 0x1A,
	MAX17050_FG_MinMaxVolt	= 0x1B,
	MAX17050_FG_MinMaxCurr	= 0x1C,
	MAX17050_FG_CONFIG		= 0x1D,
	MAX17050_FG_ICHGTerm	= 0x1E,
	MAX17050_FG_AvCap		= 0x1F,
	MAX17050_FG_ManName		= 0x20,
	MAX17050_FG_DevName		= 0x21,
	MAX17050_FG_QRtable10	= 0x22,
	MAX17050_FG_FullCAPNom	= 0x23,
	MAX17050_FG_TempNom		= 0x24,
	MAX17050_FG_TempLim		= 0x25,
	MAX17050_FG_AvgTA0		= 0x26,
	MAX17050_FG_AIN			= 0x27,
	MAX17050_FG_LearnCFG	= 0x28,
	MAX17050_FG_SHFTCFG		= 0x29,
	MAX17050_FG_RelaxCFG	= 0x2A,
	MAX17050_FG_MiscCFG		= 0x2B,
	MAX17050_FG_TGAIN		= 0x2C,
	MAX17050_FG_TOFF		= 0x2D,
	MAX17050_FG_CGAIN		= 0x2E,
	MAX17050_FG_COFF		= 0x2F,

	MAX17050_FG_dV_acc		= 0x30,
	MAX17050_FG_I_acc		= 0x31,
	MAX17050_FG_QRtable20	= 0x32,
	MAX17050_FG_MaskSOC		= 0x33,
	MAX17050_FG_CHG_CNFG_10	= 0x34,
	MAX17050_FG_FullCAP0	= 0x35,
	MAX17050_FG_Iavg_empty	= 0x36,
	MAX17050_FG_FCTC		= 0x37,
	MAX17050_FG_RCOMP0		= 0x38,
	MAX17050_FG_TempCo		= 0x39,
	MAX17050_FG_V_empty		= 0x3A,
	MAX17050_FG_AvgCurrent0	= 0x3B,
	MAX17050_FG_TaskPeriod	= 0x3C,
	MAX17050_FG_FSTAT		= 0x3D,
	MAX17050_FG_TIMER       = 0x3E,
	MAX17050_FG_SHDNTIMER	= 0x3F,

	MAX17050_FG_AvgCurrentL	= 0x40,
	MAX17050_FG_AvgTAL		= 0x41,
	MAX17050_FG_QRtable30	= 0x42,
	MAX17050_FG_RepCapL		= 0x43,
	MAX17050_FG_AvgVCELL0	= 0x44,
	MAX17050_FG_dQacc		= 0x45,
	MAX17050_FG_dp_acc		= 0x46,
	MAX17050_FG_RlxSOC		= 0x47,
	MAX17050_FG_VFSOC0		= 0x48,
	MAX17050_FG_RemCapL		= 0x49,
	MAX17050_FG_VFRemCap	= 0x4A,
	MAX17050_FG_AvgVCELLL	= 0x4B,
	MAX17050_FG_QH0			= 0x4C,
	MAX17050_FG_QH			= 0x4D,
	MAX17050_FG_QL			= 0x4E,
	MAX17050_FG_RemCapL0	= 0x4F,
	MAX17050_FG_LOCK_I		= 0x62,
	MAX17050_FG_LOCK_II		= 0x63,
	MAX17050_FG_OCV 		= 0x80,
};


enum {
	QRTABLE_IS_NEED_CHANGED,
	QRTABLE00_0DC,
	QRTABLE10_0DC,
	QRTABLE00_N3DC,
	QRTABLE10_N3DC,
};

void battery_capacity_update(struct battery_type *battery, int capacity_01p);
BOOL battery_param_update(struct battery_type *battery, struct protect_flags_type *flags);
DWORD BAHW_MyGetMSecs(void);
BOOL battery_param_init(struct battery_type *battery);
int max17050_gauge_init(void);
void max17050_gauge_exit(void);
int max17050_get_batt_level(struct battery_type *battery);
int max17050_i2c_write(u8 addr, const u8 *values, size_t len);
int max17050_i2c_read(u8 addr, u8 *values, size_t len);
int max17050_get_voltage(int *val);
int max17050_get_current(int *val);
int max17050_get_temperature(int *val);

extern void max17050_adjust_qrtable_by_temp(u16* batt_qrtable_param, s32 tempdc);

#endif 
