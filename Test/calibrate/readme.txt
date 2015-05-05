calibrate utility :: 2015-05-05
Author: edgo

The calibrate utility will make a best effort to calibrate RP
to within 1 millivolt at 0.0V, 0.8V, and -0.8V on channels 1 and 2.
The user will be prompted to read and enter voltages from digital
multimeter into the program. This process will calibrate the back
end, the front end calibration is fully automated via the program.
Once channel 1 calibration is completed, the user will be prompted
to switch the cable and termination to channel 2, and the process
will be repeated. Final results are printed at the end.

Caveats:
1) This will only work for the default High gain (LV) jumper
configuration. Of course the code may be modified for the Low gain
(HV) jumper configuration.
2) Dependent at this time on the default calibration coefficients
that RP ships with (seen with "calib -r -v"). If these defaults
ever change then this code will need to change accordingly.
3) This utility does not replace the 'calib' utility, but in fact
uses the 'calib' utility for reading/writing the EEPROM.
4) You really must enter the correct values that you read from the
DMM. Not responsible if you get that wrong.

Requirements:
1) MUST HAVE the calibration-aware versions of generate and acquire
installed in /opt/bin directory
2) Cable run from OUT1 to IN1 with 50 ohm termination in series
3) Digital Multimeter

Building:
Same as it ever was ...
> make CROSS_COMPILE=arm-linux-gnueabi- clean all

Running:
You may copy to /tmp directory and run from there:
> scp calibrate root@ip-address-of-rp:/tmp

A Sample run:

redpitaya> ./calibrate
### calibrate v0.1

### BE_CH1_FS calibration ###
1) Run cable from OUT1 to IN1, with 50 ohm termination in series
2) Put DMM across IN1, reading DC voltage

Press Enter when ready:
Resetting EEPROM:
> calib -d

Generating CH1 DC signal of 0.8 volts:
> generate 1 0 0 sine 0.8
Please enter DMM voltage read (like 0.802) here: 0.794

Generating CH1 DC signal of -0.8 volts:
> generate 1 0 0 sine -0.8
Please enter DMM voltage read (like -0.799) here: -0.8206
be_pk_to_pk_sum=1.614600
1 be_ratio_adjust=1.009125
2 be_ratio_adjust=43145476.000000
> calib -r | awk '{$7=43145476}1' | calib -w

Generating CH1 DC signal of 0.8 volts:
> generate 1 0 0 sine 0.8
Please enter DMM voltage read (like 0.802) here: 0.7867

Generating CH1 DC signal of -0.8 volts:
> generate 1 0 0 sine -0.8
Please enter DMM voltage read (like -0.799) here: -0.8133
be_pk_to_pk_sum=1.600000
1 be_ratio_adjust=1.000000

### BE_CH1_DC_offs calibration ###

Generating CH1 DC signal of 0.0 volts:
> generate 1 0 0 sine
Please enter DMM voltage read (like 0.0033) here: -0.0136
be_offset_adjust=-111.475410
be_offset_adjust_int=-111
> calib -r | awk '{$9=-39}1' | calib -w

Generating CH1 DC signal of 0.0 volts:
> generate 1 0 0 sine
Please enter DMM voltage read (like 0.0001) here: 0.0

Generating CH1 DC signal of 0.8 volts:
> generate 1 0 0 sine 0.8
Please enter DMM voltage read (like 0.802) here: 0.80004

Generating CH1 DC signal of -0.8 volts:
> generate 1 0 0 sine -0.8
Please enter DMM voltage read (like -0.799) here: -0.7997

### FE_CH1_FS_G_HI calibration ###

Generating CH1 DC signal of 0.8 volts:
> generate 1 0 0 sine 0.8
> acquire -c 1000 | awk '{print $1}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# 0.764541 0.764804 0.764761 0.764855 0.764838 0.765097 0.765107 0.765182
fe_positive=0.764898

Generating CH1 DC signal of -0.8 volts:
> generate 1 0 0 sine -0.8
> acquire -c 1000 | awk '{print $1}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# -0.763434 -0.762830 -0.763361 -0.762820 -0.763204 -0.763081 -0.763467 -0.763294
fe_negative=-0.763186
fe_pk_to_pk_sum=1.528085
1 fe_ratio_adjust=1.047063
2 fe_ratio_adjust=48029336.000000
> calib -r | awk '{$1=48029336}1' | calib -w
> acquire -c 1000 | awk '{print $1}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# -0.799389 -0.799307 -0.799241 -0.798920 -0.799122 -0.799156 -0.798707 -0.799297
fe_negative_after=-0.799142

Generating CH1 DC signal of 0.8 volts:
> generate 1 0 0 sine 0.8
> acquire -c 1000 | awk '{print $1}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# 0.800756 0.800803 0.800945 0.800895 0.800596 0.800757 0.800448 0.800564
fe_positive_after=0.800720
fe_sum_after=1.599863 (should be very close to 1.600)
error is -0.000137

### FE_CH1_DC_offs calibration ###

Generating CH1 DC signal of 0.0 volts:
> generate 1 0 0 sine
# 0.000157 -0.000367 -0.000121 -0.000272 0.000828 0.000952 -0.000270 -0.000110
> calib -r | awk '{$5=78}1' | calib -w
# 0.000868 0.000739 -0.000603 -0.000606 -0.000195 -0.000254 -0.000224 0.000807
FINAL fe_offset=0.000066
DONE LOOPING

Generating CH1 DC signal of -0.8 volts:
> generate 1 0 0 sine -0.8
> acquire -c 1000 | awk '{print $1}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# -0.799188 -0.799622 -0.799485 -0.799317 -0.799075 -0.799347 -0.798906 -0.799364
FINAL fe_negative_after=-0.799288

Generating CH1 DC signal of 0.8 volts:
> generate 1 0 0 sine 0.8
> acquire -c 1000 | awk '{print $1}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# 0.800249 0.800697 0.800969 0.800444 0.800530 0.800743 0.800673 0.800838
FINAL fe_positive_after=0.800643

!!! SWITCH PHYSICAL CABLE NOW !!!

### BE_CH2_FS calibration ###
1) Run cable from OUT2 to IN2, with 50 ohm termination in series
2) Put DMM across IN2, reading DC voltage

Press Enter when ready:
Generating CH2 DC signal of 0.8 volts:
> generate 2 0 0 sine 0.8
Please enter DMM voltage read (like 0.802) here: 0.7991

Generating CH2 DC signal of -0.8 volts:
> generate 2 0 0 sine -0.8
Please enter DMM voltage read (like -0.799) here: -0.8232
be_pk_to_pk_sum=1.622300
1 be_ratio_adjust=1.013937
2 be_ratio_adjust=43351232.000000
> calib -r | awk '{$8=43351232}1' | calib -w

Generating CH2 DC signal of 0.8 volts:
> generate 2 0 0 sine 0.8
Please enter DMM voltage read (like 0.802) here: 0.788

Generating CH2 DC signal of -0.8 volts:
> generate 2 0 0 sine -0.8
Please enter DMM voltage read (like -0.799) here: -0.8124
be_pk_to_pk_sum=1.600400
1 be_ratio_adjust=1.000250

### BE_CH2_DC_offs calibration ###

Generating CH2 DC signal of 0.0 volts:
> generate 2 0 0 sine
Please enter DMM voltage read (like 0.0033) here: -0.0121
be_offset_adjust=-99.180328
be_offset_adjust_int=-99
> calib -r | awk '{$10=-51}1' | calib -w

Generating CH2 DC signal of 0.0 volts:
> generate 2 0 0 sine
Please enter DMM voltage read (like 0.0001) here: 0.0

Generating CH2 DC signal of 0.8 volts:
> generate 2 0 0 sine 0.8
Please enter DMM voltage read (like 0.802) here: 0.800

Generating CH2 DC signal of -0.8 volts:
> generate 2 0 0 sine -0.8
Please enter DMM voltage read (like -0.799) here: -0.800

### FE_CH2_FS_G_HI calibration ###

Generating CH2 DC signal of 0.8 volts:
> generate 2 0 0 sine 0.8
> acquire -c 1000 | awk '{print $2}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# 0.750563 0.752010 0.751788 0.751756 0.750597 0.750828 0.750751 0.750888
fe_positive=0.751148

Generating CH2 DC signal of -0.8 volts:
> generate 2 0 0 sine -0.8
> acquire -c 1000 | awk '{print $2}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# -0.768435 -0.768537 -0.768291 -0.768294 -0.768269 -0.768367 -0.768735 -0.768313
fe_negative=-0.768405
fe_pk_to_pk_sum=1.519553
1 fe_ratio_adjust=1.052941
2 fe_ratio_adjust=48299004.000000
> calib -r | awk '{$2=48299004}1' | calib -w
> acquire -c 1000 | awk '{print $2}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# -0.809276 -0.809033 -0.809103 -0.809095 -0.809232 -0.808807 -0.809262 -0.808817
fe_negative_after=-0.809078

Generating CH2 DC signal of 0.8 volts:
> generate 2 0 0 sine 0.8
> acquire -c 1000 | awk '{print $2}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# 0.790076 0.790262 0.790285 0.790516 0.790402 0.791809 0.790366 0.790633
fe_positive_after=0.790544
fe_sum_after=1.599622 (should be very close to 1.600)
error is -0.000378

### FE_CH2_DC_offs calibration ###

Generating CH2 DC signal of 0.0 volts:
> generate 2 0 0 sine
# -0.010116 -0.010347 -0.010413 -0.010107 -0.010175 -0.010010 -0.010150 -0.010481
> calib -r | awk '{$6=108}1' | calib -w
# 0.001472 0.001142 0.001302 0.000893 0.001149 0.001124 0.000874 0.000781
FINAL fe_offset=0.001092
# -0.010509 -0.010123 -0.009895 -0.010184 -0.010254 -0.010018 -0.010431 -0.010379
> calib -r | awk '{$6=104}1' | calib -w
# 0.000152 0.000419 0.000811 0.000323 0.000400 0.000866 0.000682 0.000184
FINAL fe_offset=0.000480
DONE LOOPING

Generating CH2 DC signal of -0.8 volts:
> generate 2 0 0 sine -0.8
> acquire -c 1000 | awk '{print $2}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# -0.798640 -0.798605 -0.798173 -0.798288 -0.798330 -0.798376 -0.798292 -0.798306
FINAL fe_negative_after=-0.798376

Generating CH2 DC signal of 0.8 volts:
> generate 2 0 0 sine 0.8
> acquire -c 1000 | awk '{print $2}' | awk '{ total += $NF } END { printf("%f\n", total/NR) }'
# 0.802333 0.802517 0.801304 0.801173 0.801006 0.801364 0.802196 0.802282
FINAL fe_positive_after=0.801772

### CHANNEL 1
be_v1_final=0.800040
be_v2_final=-0.799700
be_offset=0.000000
fe_positive_after=0.800643
fe_negative_after=-0.799288
fe_offset=0.000066

### CHANNEL 2
be_v1_final=0.800000
be_v2_final=-0.800000
be_offset=0.000000
fe_positive_after=0.801772
fe_negative_after=-0.798376
fe_offset=0.000480
redpitaya>

