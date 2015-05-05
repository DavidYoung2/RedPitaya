CC=$(CROSS_COMPILE)gcc
RM=rm

OBJECTS=main.o fpga.o worker.o dsp.o waterfall.o
#OBJECTS=main.o fpga.o worker.o dsp.o

FFT_DIR=./external/kiss_fft
FFT_OBJECTS=$(FFT_DIR)/kiss_fft.o $(FFT_DIR)/kiss_fftr.o
FFT_INC=-I$(FFT_DIR)

JPEG_DIR=./external/jpeg-6b
JPEG_LIB=$(JPEG_DIR)/libjpeg.a
JPEG_INC=-I$(JPEG_DIR)

INCLUDE=$(FFT_INC) $(JPEG_INC)
#INCLUDE=$(FFT_INC)

# Additional libraries which needs to be dynamically linked to the executable
# -lm - System math library (used by cos(), sin(), sqrt(), ... functions)
LIBS=-lm -lpthread

CFLAGS+= -Wall -Werror -g -fPIC $(INCLUDE)
#LDFLAGS=-shared
LDFLAGS=

OUT_DIR=./
C_OUT_NAME=$(OUT_DIR)acquirefft

all: make_c_app
clean: clean_c_app clean_ext

$(JPEG_LIB):
	$(MAKE) -C $(JPEG_DIR)

$(FFT_OBJECTS):
	$(MAKE) -C $(FFT_DIR)

make_c_app: $(FFT_OBJECTS) $(JPEG_LIB) $(OBJECTS)
	$(CC) -o $(C_OUT_NAME) $(OBJECTS) $(FFT_OBJECTS) $(CFLAGS) $(LDFLAGS) $(LIBS) $(JPEG_LIB)
#make_c_app: $(FFT_OBJECTS) $(OBJECTS)
#	$(CC) -o $(C_OUT_NAME) $(OBJECTS) $(FFT_OBJECTS) $(CFLAGS) $(LDFLAGS)

clean_c_app:
	$(RM) -f $(C_OUT_NAME) $(OBJECTS)

clean_ext:
	$(MAKE) -C $(FFT_DIR) clean
	$(MAKE) -C $(JPEG_DIR) clean
