typedef struct  {
	int record;
	int band;
	int iq_offset;
	int iq_phase;
}iq_stack;

extern int update_iq_ini_file();
extern int Check_iq_ini_file();
extern void IQ_calc(int iq_int);
extern int IQ_phase_calc(int phase_steps);
extern int init_IQ_structure();