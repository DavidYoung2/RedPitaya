/**
 * $Id: calibrate.c $
 * edgo
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

float valueFromAverages(char *cmd)
{
  float val, retval = 0.0;
  int count = 8;

  printf("#");
  for (int i = 0; i < count; i++)
  {
    FILE *fh = popen(cmd, "r");
    char buf[256];
    while (fgets(buf, sizeof(buf), fh) != 0) {
    }
    pclose(fh);
    val = atof(buf);
    printf(" %f", val);
    retval += val;
  }
  printf("\n");
  return retval / count;
}

int main(int argc, char *argv[])
{
  char cmd[128];
  //
  float be_v1, be_v2, be_pk_to_pk_sum, be_ratio_adjust;
  float be_v1_final, be_v2_final;
  float be_offset, be_offset_adjust;
  int be_offset_adjust_int, be_offset_final;
  //
  float fe_positive, fe_negative, fe_pk_to_pk_sum, fe_ratio_adjust;
  float fe_positive_after, fe_negative_after;
  float fe_sum_after;
  float fe_offset, fe_offset_adjust;
  int fe_offset_adjust_int, fe_offset_final;
  //
  int looping, retval, base;
  //
  float ch1_be_v1_final, ch1_be_v2_final, ch1_be_offset;
  float ch1_fe_positive_after, ch1_fe_negative_after, ch1_fe_offset;

  printf("### calibrate v0.2\n\n");

  for (int i = 0; i < 2; i++) {

  if (i == 1)
    printf("\n!!! SWITCH PHYSICAL CABLE NOW !!!\n\n");

  printf("### BE_CH%d_FS calibration ###\n", i+1);
  printf("1) Run cable from OUT%d to IN%d, with 50 ohm termination in series\n", i+1, i+1);
  printf("2) Put DMM across IN%d, reading DC voltage\n\n", i+1);
  printf("Press Enter when ready: ");
  getchar();

  if (i == 0)
  {
    printf("Resetting EEPROM:\n");
    strcpy(cmd, "calib -d");
    printf("> %s\n", cmd);
    system(cmd);
  }

  printf("\nGenerating CH%d DC signal of 0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine 0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like 0.802) here: ");
    retval = scanf("%f", &be_v1);
    if (retval == 1)
    {
      if (be_v1 > 0.0) { looping = 0; } else { printf("This value is not positive, try again\n"); be_v1 = -0.1f; }
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  printf("\nGenerating CH%d DC signal of -0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine -0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like -0.799) here: ");
    retval = scanf("%f", &be_v2);
    if (retval == 1)
    {
      if (be_v2 < 0.0) { looping = 0; } else { printf("This value is not negative, try again\n"); be_v2 = 0.1f; }
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  be_pk_to_pk_sum = be_v1 - be_v2;
  printf("be_pk_to_pk_sum=%f\n", be_pk_to_pk_sum);
  be_ratio_adjust = be_pk_to_pk_sum / 1.6;
  printf("1 be_ratio_adjust=%f\n", be_ratio_adjust);
  be_ratio_adjust *= 42755331;

////////////////////////////////////////////////////////////////////////////////////////////

  int adjust_loop = 10;
  while (adjust_loop != 0) {
  if (adjust_loop == 1)
    be_ratio_adjust -= 4000;
  else
  if (adjust_loop == 2)
    be_ratio_adjust += 4000;
  printf("2 be_ratio_adjust=%f\n", be_ratio_adjust);
  sprintf(cmd, "calib -r | awk '{$%d=%d}1' | calib -w", i+7, (int)be_ratio_adjust);//7 or 8
  printf("> %s\n", cmd);
  system(cmd);

  printf("\nGenerating CH%d DC signal of 0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine 0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like 0.802) here: ");
    retval = scanf("%f", &be_v1);
    if (retval == 1)
    {
      if (be_v1 > 0.0) { looping = 0; } else { printf("This value is not positive, try again\n"); be_v1 = -0.1f; }
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  printf("\nGenerating CH%d DC signal of -0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine -0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like -0.799) here: ");
    retval = scanf("%f", &be_v2);
    if (retval == 1)
    {
      if (be_v2 < 0.0) { looping = 0; } else { printf("This value is not negative, try again\n"); be_v2 = 0.1f; }
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  be_pk_to_pk_sum = be_v1 - be_v2;
  printf("be_pk_to_pk_sum=%f\n", be_pk_to_pk_sum);
  //be_ratio_adjust = be_pk_to_pk_sum / 1.6;
  //printf("1 be_ratio_adjust=%f\n", be_ratio_adjust);

  printf("be_pk_to_pk_sum should be as close as possible to 1.6000\n");
  printf("0 = Good, continue, 1 = Tweek higher, 2 = Tweek lower: ");
  retval = scanf("%d", &adjust_loop);
  }//adjust_loop

/////////////////////////////////////////////////////////

  printf("\n### BE_CH%d_DC_offs calibration ###\n", i+1);
  printf("\nGenerating CH%d DC signal of 0.0 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like 0.0033) here: ");
    retval = scanf("%f", &be_offset);
    if (retval == 1)
    {
      looping = 0;
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  be_offset_adjust = be_offset / 0.000122;
  printf("be_offset_adjust=%f\n", be_offset_adjust);
  be_offset_adjust_int = (int)be_offset_adjust;
  printf("be_offset_adjust_int=%d\n", be_offset_adjust_int);
  be_offset_final = -150 - be_offset_adjust_int;

///////////////////////////////////////////////////////////////////////////////////////

  adjust_loop = 10;
  while (adjust_loop != 0) {
  if (adjust_loop == 1)
    be_offset_final += 1;
  else
  if (adjust_loop == 2)
    be_offset_final -= 1;
  sprintf(cmd, "calib -r | awk '{$%d=%d}1' | calib -w", i+9, be_offset_final);//9 or 10
  printf("> %s\n", cmd);
  system(cmd);

  printf("\nGenerating CH%d DC signal of 0.0 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like 0.0001) here: ");
    retval = scanf("%f", &be_offset);
    if (retval == 1)
    {
      looping = 0;
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }
  printf("The Zero offset should be as close as possible to 0.0000\n");
  printf("0 = Good, continue, 1 = Tweek higher, 2 = Tweek lower: ");
  retval = scanf("%d", &adjust_loop);
  }//adjust_loop

/////////////////////////////////////////////////////////////

  printf("\nGenerating CH%d DC signal of 0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine 0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like 0.802) here: ");
    retval = scanf("%f", &be_v1_final);
    if (retval == 1)
    {
      if (be_v1_final > 0.0) { looping = 0; } else { printf("This value is not positive, try again\n"); be_v1_final = -0.1f; }
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  printf("\nGenerating CH%d DC signal of -0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine -0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  looping = 1;
  while (looping == 1)
  {
    printf("Please enter DMM voltage read (like -0.799) here: ");
    retval = scanf("%f", &be_v2_final);
    if (retval == 1)
    {
      if (be_v2_final < 0.0) { looping = 0; } else { printf("This value is not negative, try again\n"); be_v2_final = 0.1f; }
    }
    else
    {
      printf("!!! Bad input, try again\n");
      getchar();
    }
  }

  printf("\n### FE_CH%d_FS_G_HI calibration ###\n", i+1);
  printf("\nGenerating CH%d DC signal of 0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine 0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  //
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  printf("> %s\n", cmd);
  //FILE *fh;
  //char buf[256];
  /***
  FILE *fh = popen(cmd, "r");
  char buf[256];
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  //printf("buf=[%s]", buf);
  fe_positive = atof(buf);
  printf("fe_positive=%f\n", fe_positive);
  //RUN CMD AGAIN
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  fe_positive = atof(buf);
  ***/
  fe_positive = valueFromAverages(cmd);
  printf("fe_positive=%f\n", fe_positive);

  printf("\nGenerating CH%d DC signal of -0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine -0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  //
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  //printf("buf=[%s]", buf);
  fe_negative = atof(buf);
  ***/
  fe_negative = valueFromAverages(cmd);
  printf("fe_negative=%f\n", fe_negative);

  fe_pk_to_pk_sum = fe_positive - fe_negative;
  printf("fe_pk_to_pk_sum=%f\n", fe_pk_to_pk_sum);
  fe_ratio_adjust = 1.6 / fe_pk_to_pk_sum;
  printf("1 fe_ratio_adjust=%f\n", fe_ratio_adjust);
  fe_ratio_adjust *= 45870551;
  printf("2 fe_ratio_adjust=%f\n", fe_ratio_adjust);
  sprintf(cmd, "calib -r | awk '{$%d=%d}1' | calib -w", i+1, (int)fe_ratio_adjust);//1 or 2
  printf("> %s\n", cmd);
  system(cmd);
  //
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  //printf("buf=[%s]", buf);
  fe_negative_after = atof(buf);
  ***/
  fe_negative_after = valueFromAverages(cmd);
  printf("fe_negative_after=%f\n", fe_negative_after);

  printf("\nGenerating CH%d DC signal of 0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine 0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  //
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  //printf("buf=[%s]", buf);
  fe_positive_after = atof(buf);
  ***/
  fe_positive_after = valueFromAverages(cmd);
  printf("fe_positive_after=%f\n", fe_positive_after);

  fe_sum_after = fe_positive_after - fe_negative_after;
  printf("fe_sum_after=%f (should be very close to 1.600)\n", fe_sum_after);
  printf("error is %f\n", fe_sum_after - 1.6);

  printf("\n### FE_CH%d_DC_offs calibration ###\n", i+1);
  printf("\nGenerating CH%d DC signal of 0.0 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine", i+1);
  printf("> %s\n", cmd);
  system(cmd);

  if (i == 0)
    base = 78;
  else
    base = 25;
  looping = 1;
  int count = 0;
  int inc_val = 4;
  while (looping == 1)
  {
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  //printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  //printf("buf=[%s]", buf);
  fe_offset = atof(buf);
  ***/
  fe_offset = valueFromAverages(cmd);
  //printf("fe_offset=%f\n", fe_offset);
  fe_offset_adjust = fe_offset / 0.000122;
  //printf("fe_offset_adjust=%f\n", fe_offset_adjust);
  fe_offset_adjust_int = (int)fe_offset_adjust;
  //printf("fe_offset_adjust_int=%d\n", fe_offset_adjust_int);
  fe_offset_final = base - fe_offset_adjust_int;

  sprintf(cmd, "calib -r | awk '{$%d=%d}1' | calib -w", i+5, fe_offset_final);//5 or 6
  printf("> %s\n", cmd);
  system(cmd);

  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  //printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  fe_offset = atof(buf);
  ***/
  fe_offset = valueFromAverages(cmd);
  printf("FINAL fe_offset=%f\n", fe_offset);
  if ((fe_offset > -0.001) && (fe_offset < 0.001))
  {
    if ((fe_offset > -0.0001) && (fe_offset < 0.0005))
    {
      printf("DONE LOOPING\n");
      looping = 0;
    }
    else
      inc_val = 2;
  }
  if (fe_offset < 0.0) base += inc_val; else base -= inc_val;
  if (looping == 1)
  {
    //reset to original value
    if (i == 0)
      sprintf(cmd, "calib -r | awk '{$5=78}1' | calib -w");
    else
      sprintf(cmd, "calib -r | awk '{$6=25}1' | calib -w");
    //printf("> %s\n", cmd);
    system(cmd);
  }

  count++;
  if (count > 20) looping = 0;
  }//end of while loop

  printf("\nGenerating CH%d DC signal of -0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine -0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  //
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  fe_negative_after = atof(buf);
  ***/
  fe_negative_after = valueFromAverages(cmd);
  printf("FINAL fe_negative_after=%f\n", fe_negative_after);

  printf("\nGenerating CH%d DC signal of 0.8 volts:\n", i+1);
  sprintf(cmd, "generate %d 0 0 sine 0.8", i+1);
  printf("> %s\n", cmd);
  system(cmd);
  //
  //sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"%f\\n\", total/NR) }'", i+1);
  sprintf(cmd, "acquire -c 1000 | awk '{print $%d}' | awk '{ total += $NF } END { printf(\"", i+1);
  strcat(cmd, "%f\\n\", total/NR) }'");
  printf("> %s\n", cmd);
  /***
  fh = popen(cmd, "r");
  while (fgets(buf, sizeof(buf), fh) != 0) {
  }
  pclose(fh);
  fe_positive_after = atof(buf);
  ***/
  fe_positive_after = valueFromAverages(cmd);
  printf("FINAL fe_positive_after=%f\n", fe_positive_after);

  if (i == 0)
  {
    ch1_be_v1_final = be_v1_final;
    ch1_be_v2_final = be_v2_final;
    ch1_be_offset = be_offset;
    //
    ch1_fe_positive_after = fe_positive_after;
    ch1_fe_negative_after = fe_negative_after;
    ch1_fe_offset = fe_offset;
  }

  } // loop of CH1 and CH2

  printf("\n### CHANNEL 1\n");
  printf("be_v1_final=%f\n", ch1_be_v1_final);
  printf("be_v2_final=%f\n", ch1_be_v2_final);
  printf("be_offset=%f\n", ch1_be_offset);
  printf("fe_positive_after=%f\n", ch1_fe_positive_after);
  printf("fe_negative_after=%f\n", ch1_fe_negative_after);
  printf("fe_offset=%f\n", ch1_fe_offset);

  printf("\n### CHANNEL 2\n");
  printf("be_v1_final=%f\n", be_v1_final);
  printf("be_v2_final=%f\n", be_v2_final);
  printf("be_offset=%f\n", be_offset);
  printf("fe_positive_after=%f\n", fe_positive_after);
  printf("fe_negative_after=%f\n", fe_negative_after);
  printf("fe_offset=%f\n", fe_offset);
}

