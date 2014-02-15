#ifndef __ASM_ARCH_MACH_MSM_HTC_VERSOIN_H
#define __ASM_ARCH_MACH_MSM_HTC_VERSOIN_H

#define HTC_PCBID_EVT_MIN   0   
#define HTC_PCBID_EVT_MAX   7   
#define HTC_PCBID_PVT_MIN   128 
#define HTC_PCBID_PVT_MAX   135 

#define BOARD_UNKNOWN   (-1)
#define BOARD_EVM       0

#define BOARD_CPEDUG_EVT_XA   1 
#define BOARD_CPEDCG_EVT_XA   1 
#define BOARD_CPEDTG_EVT_XA   1 
#define BOARD_CPEU_EVT_XA     1 

#define BOARD_CPEDUG_EVT_XB   2 
#define BOARD_CPEDCG_EVT_XB   2 
#define BOARD_CPEDTG_EVT_XB   2 
#define BOARD_CPEU_EVT_XB     2 

#define BOARD_CPEDUG_EVT_XC   3 
#define BOARD_CPEDCG_EVT_XC   3 
#define BOARD_CPEDTG_EVT_XC   3 
#define BOARD_CPEU_EVT_XC     3 

#define BOARD_CPEDUG_EVT_XD   4 
#define BOARD_CPEDCG_EVT_XD   4 
#define BOARD_CPEDTG_EVT_XD   4 
#define BOARD_CPEU_EVT_XD     4 

#define BOARD_CPEDUG_EVT_XE   5 
#define BOARD_CPEDCG_EVT_XE   5 
#define BOARD_CPEDTG_EVT_XE   5 
#define BOARD_CPEU_EVT_XE     5 

#define BOARD_CPEDUG_EVT_XF   6 
#define BOARD_CPEDCG_EVT_XF   6 
#define BOARD_CPEDTG_EVT_XF   6 
#define BOARD_CPEU_EVT_XF     6 

#define BOARD_CPEDUG_EVT_XG   7 
#define BOARD_CPEDCG_EVT_XG   7 
#define BOARD_CPEDTG_EVT_XG   7 
#define BOARD_CPEU_EVT_XG     7 

#define BOARD_CPEDUG_EVT_XH   8 
#define BOARD_CPEDCG_EVT_XH   8 
#define BOARD_CPEDTG_EVT_XH   8 
#define BOARD_CPEU_EVT_XH     8 

#define BOARD_CPEDUG_DVT_A	BOARD_CPEDUG_EVT_XD
#define BOARD_CPEDTG_DVT_A	BOARD_CPEDTG_EVT_XC

#define BOARD_PVT_A     128     
#define BOARD_PVT_B     129     
#define BOARD_PVT_C     130     
#define BOARD_PVT_D     131     
#define BOARD_PVT_E     132     
#define BOARD_PVT_F     133     
#define BOARD_PVT_G     134     
#define BOARD_PVT_H     135     

#if 0
#define BOARD_EVT_XB    2       
#define BOARD_EVT_XC    3       
#define BOARD_EVT_XD    4       
#define BOARD_EVT_XE    5       
#define BOARD_EVT_XF    6       
#define BOARD_EVT_XG    7       
#define BOARD_EVT_XH    8       

#define BOARD_PVT_A     128     
#define BOARD_PVT_B     129     
#define BOARD_PVT_C     130     
#define BOARD_PVT_D     131     
#define BOARD_PVT_E     132     
#define BOARD_PVT_F     133     
#define BOARD_PVT_G     134     
#define BOARD_PVT_H     135     
#endif

int htc_get_board_revision(void);

#endif

