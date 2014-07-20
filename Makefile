#############################################################################
# Config BBB uploads
#############################################################################
BBB_IP 		:= 192.168.7.2
BBB_USER	:= root
BBB_PASS	:= '\'
BBB_PATH	:= /pong

#############################################################################
# Make config
#############################################################################            
LDFLAGS 		:= -lrt -lm -lpthread -lv4l2
CFLAGS 			:= -O3 -g3 -Wall -c -pthread -march=armv7-a -mtune=cortex-a8 -mfpu=neon -ftree-vectorize -ffast-math -mfloat-abi=softfp
CC 				:= gcc  
BUILD_DIR 		:= Debug/
BUILD_OBJ := $(BUILD_DIR)obj/

# adjust the build path for either 'bbb' or 'upload' targets
ifeq (bbb, $(findstring bbb,$(MAKECMDGOALS)))
CC 			:= arm-linux-gnueabi-gcc-4.7
#use arm-linux-gnueabi-gcc-4.7 for Angstrom
#use arm-linux-gnueabihf-gcc-4.7 for Debian
BUILD_DIR 	:= Beaglebone/
BUILD_OBJ	:= $(BUILD_DIR)obj/
endif
ifeq ($(MAKECMDGOALS),upload)
CC 			:= arm-linux-gnueabi-gcc-4.7
#use arm-linux-gnueabi-gcc-4.7 for Angstrom
#use arm-linux-gnueabihf-gcc-4.7 for Debian
BUILD_DIR 	:= Beaglebone/
BUILD_OBJ	:= $(BUILD_DIR)obj/
endif

#############################################################################
# Add folders with include files to this PATH
#############################################################################            
INCLUDE_PATH 	:= -I"src/helpers" -I"src/test" -I"src/pong" 

#############################################################################
# Add .c files that have a main() function here
#############################################################################            
BINARIES =	src/pong/main.c
BINARIES +=	src/test/kicker-test.c
BINARIES +=	src/test/testfinal1.c	
BINARIES +=	src/test/test_gpio.c		
BINARIES +=	src/test/iofix.c	
BINARIES +=	src/test/test_limit.c
#############################################################################
# Add all other .c files here
#############################################################################            
SOURCES =	src/pong/actuator.c
SOURCES +=	src/pong/kicker.c
SOURCES +=  src/pong/centroid.c
SOURCES +=  src/pong/green-filter.c
SOURCES +=  src/pong/paddle-tracker.c
SOURCES +=  src/pong/pong.c
SOURCES +=  src/pong/red-filter.c
SOURCES +=  src/pong/trajectory.c
SOURCES +=  src/pong/videocap.c
SOURCES +=	src/helpers/filters.c
SOURCES +=	src/helpers/gpio.c
SOURCES +=	src/helpers/logger.c
SOURCES +=	src/helpers/util.c
SOURCES +=	src/helpers/video.c
			
#############################################################################
# Add .h files here
#############################################################################            
HEADERS =	src/pong/pong.h
HEADERS +=	src/helpers/util.h
HEADERS +=	src/helpers/filters.h
HEADERS +=	src/helpers/logger.h
HEADERS +=	src/helpers/video.h
HEADERS +=	src/helpers/gpio.h


#############################################################################
# TARGETS
#############################################################################            
OBJECTS := $(SOURCES:.c=.o)
PROGRAMS := $(OBJECTS) $(BINARIES:.c=.o)
BINARIES := $(BINARIES:.c=)

.PHONY: clean upload mkd all bbb

all: mkd $(SOURCES) $(BINARIES)

bbb: mkd $(SOURCES) $(BINARIES)

#create output build dir if not exists
mkd:
	@mkdir -p $(BUILD_OBJ)	
	
	
#linker	
$(BINARIES): $(PROGRAMS)
	@echo ' '
	$(CC) $(addprefix $(BUILD_OBJ), $(notdir $(OBJECTS))) \
	      $(BUILD_OBJ)$(notdir $(basename $@)).o $(LDFLAGS) \
	      -o $(BUILD_DIR)$(subst main,pong,$(notdir $(basename $@)))

#compiler
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDE_PATH) $< -o $(BUILD_OBJ)$(notdir $@)

#clean 
clean:
	rm -r $(BUILD_DIR)*

#BBB upload
upload: bbb    
	@echo 'Uploading...' 
	@echo "put -p Beaglebone/*" | sshpass -p $(BBB_PASS)  sftp $(BBB_USER)@$(BBB_IP):$(BBB_PATH) 
	@echo 'Uploading done.' 
 