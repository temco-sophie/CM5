#include "main.h"
#include "clock.h"

STR_WR far	WR_Roution[MAX_WR1];
STR_AR far AR_Roution[MAX_AR1];
UN_ID  far ID_Config[MAX_ID1];

U8_T far scheduel_id;

bit BIT_FLAG;  // 0 -- run schedule 
unsigned char idata current_hour;
unsigned char idata current_minute;
unsigned char idata dayofweek;
unsigned char idata previous_dayofweek=10;
unsigned int  idata dayofyear;

unsigned char far ar_state_index[2] = 0;
unsigned char far wr_state_index[3] = 0;
unsigned char far holiday1_state_index[3] = 0;
unsigned char far holiday2_state_index[3] = 0;

unsigned char far output_state_index[32];
unsigned char far schedual1_state_index[32];
unsigned char far schedual2_state_index[32];
unsigned char far first_time_schedual[32];

unsigned char far send_schedual[8];
bit received_schedual_data;
bit success_write_schedual;
bit receiving_schedual_byte;
struct time_per_day {
						unsigned char hours;
						unsigned char minutes;
					};
 
unsigned char bdata  REFRESH_STATUS = 0;
sbit WEEKLY_CHANGED				= REFRESH_STATUS ^ 0;
sbit ANNUAL_CHANGED				= REFRESH_STATUS ^ 1;
sbit ID_CHANGED			    	= REFRESH_STATUS ^ 2;

extern unsigned char T;
unsigned char far daylight_enable;
unsigned char far daylight_flag;
bit calibrated_time = 0;
bit cycle_minutes_timeout;

bit flag_send_schedual = 0;
      
unsigned char bdata               WR_FLAG = 0 ;
sbit  		WR_A_M				= WR_FLAG^7 ; 
sbit  		WR_VALUE			= WR_FLAG^6 ;  
sbit  		WR_STATE1			= WR_FLAG^5 ;
sbit  		WR_STATE2			= WR_FLAG^4 ;

unsigned char bdata               AR_FLAG = 0 ;
sbit  		AR_A_M				= AR_FLAG^7 ; 
sbit  		AR_VALUE			= AR_FLAG^6 ;  
 
unsigned char bdata               ID_FLAG = 0 ;
sbit  		ID_A_M				= ID_FLAG^7 ; 
sbit  		ID_OUTPUT			= ID_FLAG^6 ;  
sbit  		ID_STATE1			= ID_FLAG^5 ;
sbit  		ID_STATE2			= ID_FLAG^4 ;



unsigned int CRC16(unsigned char *puchMsg, unsigned char usDataLen);
void SendRelayDataToSub(unsigned char num, unsigned char *data_buffer);


void SetBit(unsigned char bit_number,unsigned char *byte)
{
	unsigned char mask; 
	mask = 0x01;
	mask = mask << bit_number;
	*byte = *byte | mask;
}
void ClearBit(unsigned char bit_number,unsigned char *byte)
{
	unsigned char mask; 
	mask = 0x01;
	mask = mask << bit_number;
 	mask = ~mask;
	*byte = *byte & mask;
}
bit GetBit(unsigned char bit_number,unsigned char *value)
{
	unsigned char mask;
	unsigned char octet_index;
	mask = 0x01;
	octet_index = bit_number >> 3;
	mask = mask << (bit_number & 0x07);
	return (bit)(*(value + octet_index) & mask);
}



void SendSchedualData(unsigned char number,bit flag)
{
	union {
			unsigned int  word;
			unsigned char byte[2];
		  }crc;
	
	unsigned char success = 3;
	bit receive_success = 0;
	unsigned char output_value;
	U8_T ort_num = 0;

	if(flag)
	{
		output_value = 1;
	}
	else
	{	
		output_value = 0;
	}
	write_parameters_to_nodes(number,184,output_value);
	Test[42] = sub_addr[number];
 
	if(success ==255 && receive_success == 0)
	{
	//	number--;
		if(flag) 
		{
			if(GetBit(sub_addr[number],output_state_index) == 0)
			{
				SetBit( sub_addr[number] & 0x07,&output_state_index[ sub_addr[number] >> 3 ]);
				ID_CHANGED = 0;
			}
		}
		else 
		{
			if(GetBit(sub_addr[number],output_state_index) == 1)
			{
				ClearBit( sub_addr[number] & 0x07,&output_state_index[ sub_addr[number] >> 3 ]);
				ID_CHANGED = 0;
			}
		}
	}

}
 
/* implement per 500ms */
void CaculateTime(void)
{
	unsigned int temp_year;
	unsigned char temp_month;
	unsigned char temp_day;


	current_minute =  RTC.Clk.min;
	current_hour =  RTC.Clk.hour;
	temp_day =  RTC.Clk.day;
	dayofweek =  RTC.Clk.week - 1;
	temp_month =  RTC.Clk.mon;
	temp_year =	 RTC.Clk.year;
	
	if(temp_month & 0x80)
	temp_year = 1900+temp_day ;
	else
	temp_year = 2000+temp_day ;
	temp_month&=0x1f;
	
	if(daylight_flag == 1)
	{
		if(current_hour >= 2)
		{
			current_hour++;
			Set_Clock(PCF_HOUR,current_hour);
			daylight_flag = 0; 
	//		write_eeprom(EEP_DAYLIGHT_STATUS, daylight_flag);			
		}
	}
	else if(daylight_flag == 2)
	{
		if(current_hour >= 2)
		{
			current_hour--;
		//	SetPCF8563(0x04,HexToBcd(current_hour));	//hour]
			Set_Clock(PCF_HOUR,current_hour);
			daylight_flag = 0; 
	//		write_eeprom(EEP_DAYLIGHT_STATUS, daylight_flag);			
		}
	}

	if(dayofweek != previous_dayofweek || calibrated_time == 1)
	{
		previous_dayofweek = dayofweek;
		calibrated_time = 0;		 
		switch(temp_month)
		{
			case 1:
				dayofyear = temp_day;
				break;
			case 2:
				dayofyear = 0x1f + temp_day;
				break;
			case 3:
				if((temp_year & 0x03 ) == 0x0 )
					dayofyear = 0x3c + temp_day;
				else
					dayofyear = 0x3b + temp_day;
				if(daylight_enable == 1 && temp_year >= 2007)
				{
					if(temp_day >= 8 && temp_day <= 14 && dayofweek == 6)	
					{
						daylight_flag = 1; 
			//			write_eeprom(EEP_DAYLIGHT_STATUS, daylight_flag);			
					}
				}				
				break;
			case 4:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear = 0x5b+temp_day;
				else
					dayofyear = 0x5a+temp_day;	
				if(daylight_enable == 1 && temp_year < 2007)
				{
					if(temp_day <= 7 && dayofweek == 6)	
					{
						daylight_flag = 1; 
			//			write_eeprom(EEP_DAYLIGHT_STATUS, daylight_flag);			
					}
				}	
				break;
			case 5:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0x79+temp_day;
				else
					dayofyear=0x78+temp_day;			
				break;
			case 6:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0x98+temp_day;
				else
					dayofyear=0x97+temp_day;			
				break;
			case 7:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0xb6+temp_day;
				else
					dayofyear=0xb5+temp_day;			
				break;
			case 8:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0xd5+temp_day;
				else
					dayofyear=0xd4+temp_day;			
				break;
			case 9:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0xf4+temp_day;
				else
					dayofyear=0xf3+temp_day;			
				break;
			case 10:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear = 0x112 + temp_day;
				else
					dayofyear = 0x111 + temp_day;
				if(daylight_enable == 1 && temp_year < 2007)
				{
					if((31 - temp_day <= 6) && dayofweek == 6)	
					{
						daylight_flag = 2; 
			//			write_eeprom(EEP_DAYLIGHT_STATUS, daylight_flag);		
					}
				}	
				break;
			case 11:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0x131+temp_day;
				else
					dayofyear=0x130+temp_day;	
				if(daylight_enable == 1 && temp_year >= 2007)
				{
					if(temp_day <= 7 && dayofweek == 6)	
					{
						daylight_flag = 2; 
		//				write_eeprom(EEP_DAYLIGHT_STATUS, daylight_flag);			
					}
				}			
				break;
			case 12:
				if( ( temp_year & 0x03 ) == 0x0 )
					dayofyear=0x14f+temp_day;
				else
					dayofyear=0x14e+temp_day;			
				break;
			default:
			break;
		}
		
		dayofyear -= 1;
	}
	
}
				

void CheckAnnualRoutines( void )
{
	unsigned char  i;
	unsigned char  mask;
//    unsigned char  mask_r;
	unsigned char  octet_index;
 //   unsigned char code *base_address;
	for( i=0; i<MAX_AR1; i++ )
	{
  		AR_FLAG = AR_Roution[i].UN.Desc.flag;
	   	if( !AR_A_M )  // auto 
		 {
			mask = 0x01;
			/* Assume bit0 from octet0 = Jan 1st */
			/* octet_index = ora_current.day_of_year / 8;*/
			octet_index = dayofyear >> 3;
			/* bit_index = ora_current.day_of_year % 8;*/
			/* bit_index = ora_current.day_of_year & 0x07;*/
			mask = mask << ( dayofyear & 0x07 );
			
		//	base_address = AR_Roution[i].Time;//ttt[i];
       		if(AR_Roution[i].Time[11]!=0xff && AR_Roution[i].Time[38]!=0xff)
			{ 
				if( AR_Roution[i].Time[octet_index] & mask )
				{
					if(GetBit(i,ar_state_index) == 0)
					{				 
						SetBit(i & 0x07,&ar_state_index[ i >> 3 ]);
						ANNUAL_CHANGED = 0;
					}
 				}	
				else
				{
					if(GetBit(i,ar_state_index) == 1)
					{					
						ClearBit(i & 0x07 , &ar_state_index[ i >> 3 ]); 
						ANNUAL_CHANGED = 0;
					}
				}
			}
			else
			{					 
				ClearBit(i & 0x07 , &ar_state_index[ i >> 3 ]); 
			}
	   	}
		else  // mannual
		{
		//if(*base_address!=0xff && *(base_address+1)!=0xff)
			if(AR_FLAG != 0xff)
			{
				if(AR_VALUE)
				{
					if(GetBit(i,ar_state_index) == 0)
					{				 
						SetBit(i & 0x07,&ar_state_index[ i >> 3 ]);
						ANNUAL_CHANGED = 0;
					}				 
				}
				else
				{
					if(GetBit(i,ar_state_index) == 1)
					{				 
						ClearBit(i & 0x07,&ar_state_index[ i >> 3 ]);
						ANNUAL_CHANGED = 0;
					}					
				}
			}
		}

	}
   
}



void CheckWeeklyRoutines(void)
{ 
	unsigned char w, i;
	signed char j,timeout = 0;
	unsigned char mask;
	//unsigned char mask_r;
	for( i=0; i<MAX_WR1; i++ )
	{
		w =  dayofweek;

		
		if( w < 0 ) w = 6;
		
		WR_FLAG = WR_Roution[i].UN.Desc.flag;

		j = WR_Roution[i].UN.Desc.holiday2;	
	
		if(j > 0 && j <= MAX_AR1)
		{
			WR_FLAG |= 0x10;
			j -= 1;
			mask = 0x01;
			mask = mask << (j & 0x07);		 
			if( ar_state_index[ j >> 3] & mask )
			{
				
				if(GetBit(i,holiday2_state_index) == 0)
				{			 
	 				SetBit( i & 0x07,&holiday2_state_index[ i >> 3 ]);
					WEEKLY_CHANGED = 0;
				}
				w = 8; 
			
			}
	    	else
			{	
				if(GetBit(i,holiday2_state_index) == 1)
				{			 
					ClearBit( i & 0x07 , &holiday2_state_index[ i >> 3 ] );
					WEEKLY_CHANGED = 0;
				}
			}
		}
		else
			ClearBit( i & 0x07 , &holiday2_state_index[ i >> 3 ] );

		j = WR_Roution[i].UN.Desc.holiday1;
	 	if(j > 0 && j<=MAX_AR1)
	 	{
			j -= 1;
			mask = 0x01;
			mask = mask << (j & 0x07);
			WR_FLAG |= 0x20;		 
			if( ar_state_index[ j >> 3] & mask )
			{ 
				
				if(GetBit(i,holiday1_state_index) == 0)
				{
					SetBit( i & 0x07,&holiday1_state_index[ i >> 3 ]);
					WEEKLY_CHANGED = 0;
				}
				w = 7;
				
			}
	    	else
			{
			 	if(GetBit(i,holiday1_state_index) == 1)
				{
					ClearBit( i & 0x07 , &holiday1_state_index[ i >> 3 ] );
					WEEKLY_CHANGED = 0;
				}
			}
		}
		else
			ClearBit( i & 0x07 , &holiday1_state_index[ i >> 3 ] );


		if( !WR_A_M ) // auto 
		{
			for(j = 7;j >= 0 ;j--)
			{
				if(j % 2 == 0)  // check ontime j = 0,2,4,6
				{
					if( WR_Roution[i].OnTime[w * 8 + j ] || WR_Roution[i].OnTime[w * 8 + j + 1])
					{
						if( current_hour > WR_Roution[i].OnTime[w * 8 + j] ) 		break;
						if( current_hour == WR_Roution[i].OnTime[w * 8 + j] )						
							if(current_minute >=WR_Roution[i].OnTime[w * 8 + j + 1])						
								break;
							
					}	
				}
				else 			// check offtime  j = 1,3,5,7
				{
					if(WR_Roution[i].OffTime[w * 8 + j - 1] || WR_Roution[i].OffTime[w * 8 + j])
					{
						if( current_hour > WR_Roution[i].OffTime[w * 8 + j - 1]) 	break;
						if( current_hour == WR_Roution[i].OffTime[w * 8 + j - 1])
							if( current_minute >= WR_Roution[i].OffTime[w * 8 + j] ) 		
								break;
					}		
				}							
			}

			if( j < 0) //if( j == 0)  current time is not in Ontime or Offtime
			{
				if(GetBit(i,wr_state_index) == 1)
				{	 
					ClearBit( i & 0x07 , &wr_state_index[ i >> 3 ] );
					WEEKLY_CHANGED = 0;									
				}
			}
			else
			{
				if( j % 2) /* current time is  off time*/
				{
					Test[26]++;
					if(GetBit(i,wr_state_index) == 1)
					{	
						ClearBit( i & 0x07 , &wr_state_index[ i >> 3 ] );
						WEEKLY_CHANGED = 0;
					} 
				}
					 
				else   /* current time is on time*/
				{
					if(GetBit(i,wr_state_index) == 0)
					{	
						SetBit( i & 0x07,&wr_state_index[ i >> 3 ]);
						WEEKLY_CHANGED = 0;
					}
	 
				}
			}

		}
		else   // manual
		{
			if(WR_FLAG != 0xff)
			{
				if(WR_VALUE)
				{
					if(GetBit(i,wr_state_index) == 0)
					{	 
						SetBit( i & 0x07,&wr_state_index[ i >> 3 ]);
						WEEKLY_CHANGED = 0;
					}
				}
				else
				{
					if(GetBit(i,wr_state_index) == 1)
					{	 
						ClearBit( i & 0x07 ,&wr_state_index[ i >> 3 ]);
						WEEKLY_CHANGED = 0;
					}
				}
			}
		}
 	}	
}



void CheckIdRoutines(void)
{
	unsigned char i;
//    unsigned char code *base_address;
    bit wr1_value=0;
    bit wr2_value=0;
	bit output_value=0;
	bit temp_bit;
	bit wr1_valid;
	bit wr2_valid;
//	for(i = 0;i < MAX_ID1;i++)
	static char j = 0;
	
	//for(j = 0;j < sub_no + switch_sub_no;j++)
	{
		if(sub_addr[j] > 1)	
		{
			i = sub_addr[j] - 1;
			scheduel_id = j;
		}
		else
		{
			j++;	
			i = 0;		
		}
		if(ID_Config[i].all != NO_DEFINE_ADDRESS && i != 0)
		{
			ID_FLAG = ID_Config[i].Str.flag;
			wr1_valid = 0;
			wr2_valid = 0;
		//	test0 = ID_Config[i].Str.schedule1;
			if(ID_Config[i].Str.schedule1 > 0 && ID_Config[i].Str.schedule1 <= MAX_WR1)
			{
				if(GetBit(ID_Config[i].Str.schedule1 - 1,wr_state_index))
				{ 
					wr1_value=1;
					if(GetBit(i,schedual1_state_index) == 0)
					{
						SetBit( i & 0x07,&schedual1_state_index[ i >> 3 ]);
						ID_CHANGED = 0;
					}
				}
				else
				{
					wr1_value=0;
					if(GetBit(i,schedual1_state_index) == 1)
					{
						ClearBit(i & 0x07 ,&schedual1_state_index[ i >> 3 ]);
						ID_CHANGED = 0;
					}
				}
				wr1_valid = 1;
			}
			else
				ClearBit(i & 0x07 ,&schedual1_state_index[ i >> 3 ]);

			if(ID_Config[i].Str.schedule2 > 0 && ID_Config[i].Str.schedule2 <= MAX_WR1)
			{
				if(GetBit(ID_Config[i].Str.schedule2 - 1,wr_state_index))
				{ 
					wr2_value = 1;
					if(GetBit(i,schedual2_state_index) == 0)
					{
						SetBit(i & 0x07,&schedual2_state_index[ i >> 3 ]);
						ID_CHANGED = 0;
					}
				}
				else
				{
					wr2_value = 0;
					if(GetBit(i,schedual2_state_index) == 1)
					{
						ClearBit(i & 0x07 ,&schedual2_state_index[ i >> 3 ]);
						ID_CHANGED = 0;
					}
				}
				wr2_valid = 1;
			}
			else
				ClearBit(i & 0x07 ,&schedual2_state_index[ i >> 3 ]);


			if(!ID_A_M)  // AUTO
			{
 				if(wr1_valid || wr2_valid )
				{
				//	SendSchedualData(i,wr1_value | wr2_value);  
			#if 1
					output_value = GetBit(i,output_state_index); 
	 				temp_bit = GetBit(i,first_time_schedual);
					if(output_value != (wr1_value | wr2_value) || temp_bit == 0 )
					{ 
						if(temp_bit == 0)
						SetBit(i & 0x07 ,&first_time_schedual[ i >> 3 ]);					 
 		 				
						SendSchedualData(scheduel_id,wr1_value | wr2_value);  
					}
					else if(cycle_minutes_timeout == 1)
					{
						SendSchedualData(scheduel_id,wr1_value | wr2_value);  
						if(i == MAX_ID1 - 1)
						cycle_minutes_timeout = 0;
					}
			#endif
				}
			}
			else  // MAN
			{
				if(ID_FLAG != 0xff)
				{
				//	SendSchedualData(i,ID_OUTPUT);  
				//	test0 = 7;
					#if 1
					temp_bit = GetBit(i,first_time_schedual);
					if(ID_OUTPUT != GetBit(i,output_state_index) || temp_bit == 0)
					{ 
					//	test0 = 8;
						if(temp_bit == 0)
						SetBit(i & 0x07 ,&first_time_schedual[ i >> 3 ]);					 
						SendSchedualData(scheduel_id,ID_OUTPUT);
					}
					else if(cycle_minutes_timeout == 1)
					{
					//	test0 = 9;
						SendSchedualData(scheduel_id,ID_OUTPUT);
						if(i == MAX_ID1 -1)
						cycle_minutes_timeout = 0;
					}
					#endif
				}
			}
		}//check if the correspond ID has been used
	}//for 

	if(j < sub_no)
	{
		j++;
	}
	else
		j = 0;
}//main function





void Schedule_task(void) reentrant
{
	static U8_T	count = 0;
	portTickType xDelayPeriod = ( portTickType ) 500 / portTICK_RATE_MS;//500
//    U8_T i;   	
	for (;;)
	{ 	
		vTaskDelay(xDelayPeriod);
		Test[2]++;	

		if(sub_no == 0) continue;
				
			
		//if(!flag_transimit_from_serial)	 
		{
/* implement CaculateTime rution  per 500ms */
		CaculateTime();
/* implement CheckWeeklyRoutines rution  per 1s */
		if(count % 2 == 0)  // 1s
		{
			//Para[1]++;
			CheckWeeklyRoutines();
		}
/* implement CheckAnnualRoutines rution  per 3s */
/* implement CheckIdRoutines rution  per 3s */	
		if(count % 6 == 0)  // 3s
		{
			//Para[3]++;
			CheckAnnualRoutines();
			CheckIdRoutines();
		}

		if(count < 6) count++;
		else count = 0;
	   	}
	}
}