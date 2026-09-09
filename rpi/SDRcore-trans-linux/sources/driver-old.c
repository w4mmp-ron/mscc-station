#include "extern.h"
#include "commands.h"
#include "port_defines.h"
#include "iq.h"
#include "version.h"


#define AVERAGE_LIMIT 20
#define BAND_LIMIT 10
#define ARRAY_ELEMENTS 25
#define APPLY_LIMIT 1
#define APPLY_COUNT 3
#define BAND_CHANGE_LIMIT 6
#define IN_RUSH_COUNT 2
#define IN_RUSH_REDUCTION 0.98f
//#define TARGET_TEMP 0
//#define LIMIT_TEMP 1
//#define SCALE 0.15f

extern uint8_t G_DSP_Busy;

uint8_t G_temperature_received = FALSE;
uint8_t G_bias_received = FALSE;
float G_VU_ratio = 0.0f;

float average_temperature[AVERAGE_LIMIT] = {0.0f};
float drive_level_array[AVERAGE_LIMIT] = {0.0f};
float smoothing_average_array[AVERAGE_LIMIT] = {0.0f};
float drive_average_result_array[AVERAGE_LIMIT] = {0.0f};
uint32_t bias_array[AVERAGE_LIMIT] = {0};

float G_drive_average_result = 0.0f;
float previous_temperature = 0.0f;
const char *homedir;


float drive_coefficients_cold[BAND_LIMIT][ARRAY_ELEMENTS] = {1.0f};

float drive_coefficients_low[BAND_LIMIT][ARRAY_ELEMENTS] = {1.0f};

float drive_coefficients_mid[BAND_LIMIT][ARRAY_ELEMENTS] = {1.0f};

float drive_coefficients_high[BAND_LIMIT][ARRAY_ELEMENTS] = {1.0f};

void Initialize_coefficient_table(uint8_t table)
{
        int status = 0;

        FILE *Drive_table;
        char l_path[PATH_MAX] = {0};
        char table_record[1024];
        uint8_t band = 0;
        uint8_t element = 0;
        char *token = NULL;
        const char s[2] = ",;";
        int record_size = 0;
        char *next_token = NULL;

        if ((homedir = getenv("HOME")) != NULL) {
                strcpy(l_path, homedir);
                strcat(l_path, "/.local/share/mscc");

                switch (table) {
                case COLD_TABLE:

                        strcat(l_path, "/amplifier_calibration/drive_cold.ini");

                        break;
                case LOW_TABLE:

                        strcat(l_path, "/amplifier_calibration/drive_low.ini");

                        break;
                case MID_TABLE:

                        strcat(l_path, "/amplifier_calibration/drive_mid.ini");

                        break;
                case HIGH_TABLE:

                        strcat(l_path, "/amplifier_calibration/drive_high.ini");

                        break;
                }
                print_time();
                fprintf(G_fp_logfile, "[%d] Initialize_coefficient_table -> Table: %d, Path: %s\n", line_number++, table, l_path);
                Drive_table = fopen(l_path, "r");
                if (Drive_table != NULL) {
                        record_size = sizeof(table_record);
                        fgets(table_record, record_size, Drive_table); //Skip the first two lines
                        fgets(table_record, record_size, Drive_table);

                        while (fgets(table_record, record_size, Drive_table) != NULL) {
                                print_time();
                                fprintf(G_fp_logfile, "[%d] Initialize_coefficient_table. Band: %d -> ", line_number++, band);
                                //fprintf(G_fp_logfile, "%s", table_record);
                                token = strtok_r(table_record, s, &next_token);
                                while (token != NULL && element < ARRAY_ELEMENTS) {
                                        switch (table) {
                                        case COLD_TABLE:
                                                drive_coefficients_cold[band][element] = (float) atof(token);
                                                fprintf(G_fp_logfile, "E: %d, %f ", element, drive_coefficients_cold[band][element]);
                                                break;
                                        case LOW_TABLE:
                                                drive_coefficients_low[band][element] = (float) atof(token);
                                                fprintf(G_fp_logfile, "E: %d, %f ", element, drive_coefficients_low[band][element]);
                                                break;
                                        case MID_TABLE:
                                                drive_coefficients_mid[band][element] = (float) atof(token);
                                                fprintf(G_fp_logfile, "E: %d, %f ", element, drive_coefficients_mid[band][element]);
                                                break;
                                        case HIGH_TABLE:
                                                drive_coefficients_high[band][element] = (float) atof(token);
                                                fprintf(G_fp_logfile, "E: %d, %f ", element, drive_coefficients_high[band][element]);
                                                break;
                                        }
                                        token = strtok_r(NULL, s, &next_token);
                                        element++;
                                }
                                fprintf(G_fp_logfile, "\n");
                                next_token = NULL;
                                token = NULL;
                                band++;
                                element = 0;
                        }
                        if (Drive_table != NULL) {
                                fclose(Drive_table);
                        }

                }
        }
        //fprintf(G_fp_logfile, "\n");
}

uint32_t Get_Bias_average(uint32_t bias)
{
        uint32_t Bias_average_result = 0;
        int average_count = 0;
        uint32_t Bias_average = 0;
        int i = 0;
        static uint32_t bias_valid[3] = {0};
        static int invalid_bias_count = 0;

        bias_valid[0] = bias;
        if (((bias_valid[0] >= bias_valid[1]) && (bias_valid[1] >= bias_valid[2])) || //Bias going up
                ((bias_valid[2] >= bias_valid[1]) && (bias_valid[1] >= bias_valid[0]))) { //Bias going down
                bias_array[0] = bias;
                for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
                        Bias_average = Bias_average + bias_array[average_count];
                }
                Bias_average_result = Bias_average / average_count;
                for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
                        bias_array[i] = bias_array[(i - 1)];
                }
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Get_Bias_average -> Bias: %d \n", line_number++,Bias_average_result);
        } else {
                if (invalid_bias_count++ > 10) {
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Get_Bias_average -> Bias Not Valid \n", line_number++);
                        invalid_bias_count = 0;
                }
        }
        bias_valid[2] = bias_valid[1];
        bias_valid[1] = bias_valid[0];
        return Bias_average_result;
}

float Get_Smoothing(uint8_t new, int temperature_index, uint8_t table, uint8_t band)
{
        float smoothing_result = 0.0f;
        static float smoothing = 0.0f;
        float drive_level = 0.0f;
        float drive_level_temp = 0.0f;
        static uint8_t positive_count = 0;
        static uint8_t negative_count = 0;
        static uint8_t total_count = 0;
        static int start_temperature_index = 0;
        int start_index = 0;
        int delta_temperature = 0;
        static int index = 0;

        if (new) {
                index = temperature_index;
                start_index = temperature_index;
                positive_count = 0;
                negative_count = 0;
                total_count = 0;
                start_temperature_index = temperature_index;
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> NEW. Table: %d, index: %d, start_index: %d \n", 
                //	line_number++,table,index,start_index);
                switch (table) {
                case 0:
                        positive_count++;
                        //print_time();
                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_cold[band][index], positive_count, index);
                        drive_level = drive_coefficients_cold[band][index++];
                        do {// Go forward until the drive level changes or index = ARRAY_ELEMENTS
                                drive_level_temp = drive_coefficients_cold[band][index];
                                if (drive_level_temp == drive_level) {
                                        positive_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_cold[band][index], positive_count, index);
                                }
                        } while (drive_level == drive_level_temp && ++index < ARRAY_ELEMENTS);
                        index = start_index - 1;
                        do {// Go reverse until the drive level changes or index = 0
                                drive_level_temp = drive_coefficients_cold[band][index];
                                if (drive_level_temp == drive_level) {
                                        negative_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, N: %d, index: %d\n", drive_coefficients_cold[band][index], negative_count, index);
                                }
                        } while (drive_level == drive_level_temp && --index >= 0);
                        break;
                case 1:
                        positive_count++;
                        //print_time();
                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_low[band][index], positive_count, index);
                        drive_level = drive_coefficients_low[band][index++];
                        do {// Go forward until the drive level changes or index = ARRAY_ELEMENTS
                                drive_level_temp = drive_coefficients_low[band][index];
                                if (drive_level_temp == drive_level) {
                                        positive_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_low[band][index], positive_count, index);
                                }
                        } while (drive_level == drive_level_temp && ++index < ARRAY_ELEMENTS);
                        index = start_index - 1;
                        do {// Go reverse until the drive level changes or index = 0
                                drive_level_temp = drive_coefficients_low[band][index];
                                if (drive_level_temp == drive_level) {
                                        negative_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, N: %d, index: %d\n", drive_coefficients_low[band][index], negative_count, index);
                                }
                        } while (drive_level == drive_level_temp && --index >= 0);
                        break;

                case 2:
                        positive_count++;
                        //print_time();
                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_mid[band][index], positive_count, index);
                        drive_level = drive_coefficients_mid[band][index++];
                        do {// Go forward until the drive level changes or index = ARRAY_ELEMENTS
                                if (index < ARRAY_ELEMENTS) {
                                        drive_level_temp = drive_coefficients_mid[band][index];
                                        if (drive_level_temp == drive_level) {
                                                positive_count++;
                                                //print_time();
                                                //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_mid[band][index], positive_count, index);
                                        }
                                }
                        } while (drive_level == drive_level_temp && ++index < ARRAY_ELEMENTS);
                        index = start_index - 1;
                        do {// Go reverse until the drive level changes or index = 0
                                drive_level_temp = drive_coefficients_mid[band][index];
                                if (drive_level_temp == drive_level) {
                                        negative_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, N: %d, index: %d\n", drive_coefficients_mid[band][index], negative_count, index);
                                }
                        } while (drive_level == drive_level_temp && --index >= 0);
                        break;

                case 3:
                        positive_count++;
                        //print_time();
                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_high[band][index], positive_count, index);
                        drive_level = drive_coefficients_high[band][index++];
                        do {// Go forward until the drive level changes or index = ARRAY_ELEMENTS
                                drive_level_temp = drive_coefficients_high[band][index];
                                if (drive_level_temp == drive_level) {
                                        positive_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, P: %d, index: %d\n", drive_coefficients_high[band][index], positive_count, index);
                                }
                        } while (drive_level == drive_level_temp && ++index < ARRAY_ELEMENTS);
                        index = start_index - 1;
                        do {// Go reverse until the drive level changes or index = 0
                                drive_level_temp = drive_coefficients_high[band][index];
                                if (drive_level_temp == drive_level) {
                                        negative_count++;
                                        //print_time();
                                        //fprintf(G_fp_logfile, " drive: %f, N: %d, index: %d\n", drive_coefficients_high[band][index], negative_count, index);
                                }
                        } while (drive_level == drive_level_temp && --index >= 0);
                        break;
                }
                //fprintf(G_fp_logfile, "\n");
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> Negative Count: %d\n", line_number++, negative_count);
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> Positive Count: %d\n", line_number++, positive_count);
                start_temperature_index = temperature_index;
                //print_time();
                if (negative_count > 0) {
                        start_temperature_index = temperature_index - negative_count;
                }
                //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> start_temperature_index: %d\n", 
                //		line_number++, start_temperature_index);
                total_count = positive_count + negative_count;
                if (total_count > 1) {
                        smoothing = 1.0f / (float) total_count;

                }
        } else {
                if (temperature_index < start_temperature_index) {
                        delta_temperature = start_temperature_index - temperature_index;
                        smoothing_result = (float) delta_temperature * smoothing;
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> BELOW -> delta_temperature: %d\n", line_number++, delta_temperature);
                } else {
                        if (temperature_index > start_temperature_index) {
                                delta_temperature = temperature_index - start_temperature_index;
                                smoothing_result = (float) delta_temperature * smoothing;
                                //print_time();
                                //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> ABOVE -> delta_temperature: %d\n", line_number++, delta_temperature);
                        } else {
                                smoothing_result = 0.0f;
                        }
                }
        }
        smoothing_result = smoothing_result / 100.0f;
        //print_time();
        //fprintf(G_fp_logfile, "[%d] Get_Smoothing_Range -> table: %d, band: %d, start_temperature_index: %d, count:%d, smoothing_result: %f\n", 
        //	line_number++,table,band, start_temperature_index, total_count,smoothing_result);

        return smoothing_result;
}

float Set_Inrush(uint8_t on_off)
{
        float in_rush = 0.0f;
        static uint8_t in_rush_set = FALSE;
        static uint8_t in_rush_count = 0;

        switch (on_off) {
        case TRUE:
                if (++in_rush_count <= IN_RUSH_COUNT) {
                        in_rush = IN_RUSH_REDUCTION;
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Set_Inrush Called. Inrush set: %f\n", line_number++, in_rush);
                } else {
                        in_rush = 1.00000f;
                }
                break;
        case FALSE:
                in_rush = 1.0000000f;
                in_rush_count = 0;
        }
        return in_rush;
}

float Drive_Level(uint8_t in_rush_toggle)
{
        float drive_average = 0.0f;
        static int first_pass = TRUE;
        static int count = 0;
        static int previous_calibration_index = 20;
        static int drive_count = 0;
        static float in_rush = 0.0f;
        float temperature_average = 0.0f;
        float temperature_average_result = 0.0f;
        static double previous_temperature_d = 0.0;
        int average_count = 0;
        int temperature_i = 0;
        double temperature_d = 0.0;
        float smoothing_result = 0.0f;
        float drive_average_result = 0.0f;
        float drive_level = 0.0f;
        static float previous_drive_level = 0.0f;
        float final_drive_average = 0.0f;
        static float final_drive_average_previous = 0.0f;
        float final_drive_average_temp = 0.0f;
        int i = 0;
        uint8_t table = 0;
        static uint8_t previous_table = 0;

        if (first_pass) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Drive_Level -> First Pass \n", line_number++);
                for (count = 0; count < AVERAGE_LIMIT; count++) {
                        average_temperature[count] = G_temperature;
                }
                for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
                        temperature_average = temperature_average + average_temperature[average_count];
                }
                temperature_average_result = temperature_average / (float) average_count;
                count = 0;
                first_pass = FALSE;
        } else {
                average_temperature[0] = G_temperature;
                for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
                        temperature_average = temperature_average + average_temperature[average_count];
                }
                temperature_average_result = temperature_average / (float) average_count;
                for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
                        average_temperature[i] = average_temperature[(i - 1)];
                }
        }
        temperature_i = (int) temperature_average_result;
        if (temperature_i <= 24) {
                temperature_i = temperature_i - 10;
                if (temperature_i < 0) {
                        temperature_i = 0;
                }
                table = 1;
                drive_level = drive_coefficients_low[G_calibration_index][temperature_i];
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Drive_Level -> low table selected. Temperature index: %d, drive_level: %f\n", line_number++,
                //	temperature_i, drive_level);
        } else {
                if (temperature_i <= 49) {
                        temperature_i = temperature_i - 25;
                        if (temperature_i < 0) {
                                temperature_i = 0;
                        }
                        drive_level = drive_coefficients_mid[G_calibration_index][temperature_i];
                        table = 2;
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Drive_Level -> mid table selected. Temperature index: %d, drive_level: %f\n", line_number++,
                        //	temperature_i, drive_level);
                } else {
                        temperature_i = temperature_i - 50;
                        if (temperature_i > ARRAY_ELEMENTS - 1) {
                                temperature_i = ARRAY_ELEMENTS - 1;
                        }
                        table = 3;
                        drive_level = drive_coefficients_high[G_calibration_index][temperature_i];
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Drive_Level -> high table selected. Temperature index: %d, drive_level: %f\n",
                        //	line_number++, temperature_i, drive_level);
                }
        }
        if ((previous_calibration_index != G_calibration_index)) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Drive_Level -> Band changed Initializing drive_level_array. Band: %d\n",
                        line_number++, G_calibration_index);
                for (drive_count = 0; drive_count < AVERAGE_LIMIT; drive_count++) {
                        drive_level_array[drive_count] = drive_level;
                }
                for (drive_count = 0; drive_count < AVERAGE_LIMIT; drive_count++) {
                        drive_average = drive_average + drive_level_array[drive_count];
                }
                drive_average_result = drive_average / (float) drive_count;

                for (drive_count = 0; drive_count < AVERAGE_LIMIT; drive_count++) {
                        drive_average_result_array[drive_count] = drive_average_result;
                }
                for (drive_count = 0; drive_count < AVERAGE_LIMIT; drive_count++) {
                        final_drive_average_temp = final_drive_average_temp + drive_average_result_array[drive_count];
                }
                final_drive_average = final_drive_average_temp / (float) drive_count;
                previous_calibration_index = G_calibration_index;
                drive_count = 0;
        } else {
                drive_level_array[0] = drive_level;
                for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
                        drive_average = drive_average + drive_level_array[average_count];
                }
                drive_average_result = drive_average / (float) average_count;

                for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
                        drive_level_array[i] = drive_level_array[(i - 1)];
                }
                if (previous_drive_level != drive_level || previous_table != table) {
                        smoothing_result = Get_Smoothing(TRUE, temperature_i, table, G_calibration_index);
                        previous_drive_level = drive_level;
                        previous_table = table;
                } else {
                        smoothing_result = Get_Smoothing(FALSE, temperature_i, table, G_calibration_index);
                }
                smoothing_result = 0.0f; //TEST CODE REMOVE FOR PRODUCTION
                if (drive_average_result < 1.0000000f) {
                        drive_average_result = drive_average_result + smoothing_result;
                }

                //Now average the average drive average result after smoothing has been applied.
                drive_average_result_array[0] = drive_average_result;

                for (average_count = 0; average_count < AVERAGE_LIMIT; average_count++) {
                        final_drive_average_temp = final_drive_average_temp + drive_average_result_array[average_count];
                }
                final_drive_average = final_drive_average_temp / (float) average_count;
                for (i = (AVERAGE_LIMIT - 1); i > 0; i--) {
                        drive_average_result_array[i] = drive_average_result_array[(i - 1)];
                }
        }
        if (in_rush_toggle == TRUE) {
                in_rush = Set_Inrush(TRUE);
        } else {
                in_rush = Set_Inrush(FALSE);
        }
        //final_drive_average = final_drive_average * in_rush;
        //final_drive_average = final_drive_average; //Testing 
        //print_time();
        //fprintf(G_fp_logfile, "[%d] Drive_Level -> Temperature Average: %f, Band: %d, temperature_i (index): %d, temperature: %d,"
        //	" smoothing: %lf, drive: %f\n",
        //	line_number++, temperature_average_result, G_calibration_index, temperature_i, (int)temperature_d, smoothing_result, drive_level);

        if (final_drive_average != final_drive_average_previous) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Drive_Level -> Final Average %f, in_rush: %f\n",
                        line_number++, final_drive_average, in_rush);
                final_drive_average_previous = final_drive_average;
        }
        return final_drive_average;
}

uint8_t Set_Apply_Limit(uint8_t on_off)
{
        uint8_t apply_value = 0;
        static uint8_t apply_set = FALSE;
        static uint8_t apply_count = 0;

        switch (on_off) {
        case TRUE:
                if (++apply_count <= APPLY_COUNT) {
                        apply_value = 0;
                } else {
                        apply_value = APPLY_LIMIT;
                }
                break;
        case FALSE:
                apply_value = APPLY_LIMIT;
                apply_count = 0;
        }
        return apply_value;
}

void Set_QRO_Power()
{
        static float new_drive = 0.0f;
        static float drive_average = 0.0f;
        static float temperature_valid[3] = {0.0f};
        static uint8_t drive_average_valid = FALSE;
        static uint8_t apply_drive = 0;
        static uint8_t apply_limit = 0;
        static int previous_calibration_index = 0;
        static int8_t band_change_count = 0;
        static float configured_drive = 0.0f;
        static uint8_t in_rush = FALSE;
        static char previous_G_mode = 'N';
        static float previous_drive = 1000.0f;

        in_rush = G_tx_mode;
        if (G_temperature_received == TRUE) {
                //print_time();
                //fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> Processing \n", line_number++);
                temperature_valid[0] = G_temperature;
                if (((temperature_valid[0] >= temperature_valid[1]) && (temperature_valid[1] >= temperature_valid[2])) || //Temperature going up
                        ((temperature_valid[2] >= temperature_valid[1]) && (temperature_valid[1] >= temperature_valid[0]))) { //Temperature going down
                        drive_average_valid = TRUE;
                        drive_average = Drive_Level(in_rush);
                } else {
                        drive_average_valid = FALSE;
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> Temperature NOT VALID. Element 0: %f, Element 1: %f, Element 2: %f\n",
                        //	line_number++, temperature_valid[0], temperature_valid[1], temperature_valid[2]);
                }
                temperature_valid[2] = temperature_valid[1];
                temperature_valid[1] = temperature_valid[0];
                switch (G_mode) {
                case 'T':
                        configured_drive = G_QRO_tune_power;
                        apply_limit = 0;
                        break;
                case 'C':
                        configured_drive = G_QRO_cw_power;
                        break;
                case 'L':
                        configured_drive = G_QRO_ssb_power;
                        break;
                case 'U':
                        configured_drive = G_QRO_ssb_power;
                        break;
                case 'A':
                        configured_drive = G_QRO_am_power;
                        break;
                }

                if (previous_G_mode != G_mode || previous_drive != configured_drive) {
                        print_time();
                        fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> G_mode %c, configured_drive %f\n",
                                line_number++, G_mode, configured_drive);
                        previous_G_mode = G_mode;
                        previous_drive = configured_drive;
                }

                if (in_rush == TRUE) {
                        apply_limit = Set_Apply_Limit(TRUE);
                } else {
                        apply_limit = Set_Apply_Limit(FALSE);
                }

                if (previous_calibration_index != G_calibration_index) {
                        band_change_count = BAND_CHANGE_LIMIT;
                        previous_calibration_index = G_calibration_index;
                }
                if (band_change_count >= 0) {
                        apply_drive = APPLY_LIMIT + 1;
                        band_change_count--;
                }
                if (G_DSP_Busy == FALSE) {
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> DSP Engine is FREE\n", line_number++);
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> Set_Apply_Limit value: %d\n", line_number++, apply_limit);
                        if ((drive_average_valid == TRUE) && (apply_drive++ >= apply_limit)) {
                                new_drive = configured_drive * drive_average;
                                if (new_drive > configured_drive) {
                                        print_time();
                                        fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> New drive out of range. New_drive: %f, Configured_drive: %f\n",
                                                line_number++, new_drive, configured_drive);
                                        new_drive = configured_drive;
                                }
                                G_VU_ratio = G_mic_volume / configured_drive;
                                if (mystate.txPower != new_drive) {
                                        mystate.txPower = new_drive;
                                        print_time();
                                        fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> Applying new drive_average: %f, configured_drive: %f,"
                                                " new_drive: %f, drive ratio: %f\n",
                                                line_number++, drive_average, configured_drive, new_drive, G_VU_ratio);
                                }
                                apply_drive = 0;
                        }
                } else {
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Set_QRO_Power -> DSP Engine is BUSY\n", line_number++);
                }
        }
}

void Set_QRP_Power()
{
        static float configured_drive = 0.0f;
        static float previous_drive = 1000.0f;
        static char previous_G_mode = 'N';

        switch (G_mode) {
        case 'T':
                configured_drive = G_QRP_tune_power;
                break;
        case 'C':
                configured_drive = G_QRP_cw_power;
                break;
        case 'L':
                configured_drive = G_QRP_ssb_power;
                break;
        case 'U':
                configured_drive = G_QRP_ssb_power;
                break;
        case 'A':
                configured_drive = G_QRP_am_power;
                break;
        }
        if (mystate.txPower != configured_drive) {
                mystate.txPower = configured_drive;
        }
        G_VU_ratio = G_mic_volume / configured_drive;
        if (previous_G_mode != G_mode || previous_drive != configured_drive) {
                print_time();
                fprintf(G_fp_logfile, "[%d] Set_QRP_Power -> G_mode %c, configured_drive %f\n",
                        line_number++, G_mode, configured_drive);
                previous_G_mode = G_mode;
                previous_drive = configured_drive;
        }
}

void *Drive_Manager(void *t)
{
        uint32_t Bias_average;
        uint8_t mode = 10;

        Sleep(6000);
        print_time();
        fprintf(G_fp_logfile, "[%d] Drive_Manager -> Started after sleeping for 6 seconds \n", line_number++);

        while (G_all_threads_run) {
                if (mode != G_QRP_mode) {
                        switch (G_QRP_mode) {
                        case 0:
                                print_time();
                                fprintf(G_fp_logfile, "[%d] Drive_Manager -> In QRO mode \n", line_number++);
                                break;
                        case 1:
                                print_time();
                                fprintf(G_fp_logfile, "[%d] Drive_Manager -> In QRP mode \n", line_number++);
                                break;
                        }
                        mode = G_QRP_mode;
                }
                if (!G_QRP_mode) {
                        Set_QRO_Power();
                } else {
                        Set_QRP_Power();
                }
                if (G_bias_received == TRUE) {
                        //print_time();
                        //fprintf(G_fp_logfile, "[%d] Drive_Manager -> Finished. Bias :%d\n", line_number++, G_potentia_bias);
                        Bias_average = Get_Bias_average(G_potentia_bias);
                        G_bias_received = FALSE;
                }
                Sleep(50);
        }
        pthread_exit(0);
        return(NULL);
}