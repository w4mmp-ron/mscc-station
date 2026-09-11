typedef struct  {
	int record;
	int band;
	int iq_offset;
}iq_stack;

extern int update_iq_ini_file();
extern int create_iq_ini_file();
extern void IQ_calc(int iq_int);
extern int init_IQ_structure();