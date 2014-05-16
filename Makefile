#CUR_DIR = ../../
#ifeq ($(PARAM_FILE), ) 
#    PARAM_FILE:=$(CUR_DIR)Makefile.param
#    include $(PARAM_FILE)
#endif


SYS_BIT = 64
#SYS_BIT = `getconf LONG_BIT`

#file type
FILETYPE = c

#tool chain
#CROSS = mipsel-linux-
CC = $(CROSS)gcc
CXX = $(CROSS)g++
AR = $(CROSS)ar


ifeq ($(CROSS), )
OPENSSL_DIR = ../openssllib_x86/
JSON_DIR = ../jsonlib_x86/
else
endif


#include directory
INC_FLAGS = -I ./inc
INC_FLAGS += -I $(OPENSSL_DIR)include
INC_FLAGS += -I $(JSON_DIR)include

PROJECT = $(notdir $(CURDIR))

#INSTALL_DIR = 

#compile parameters
CDEF += -D_DEBUG

CFLAGS += -O0 -g3 -Wall $(INC_FLAGS) $(CDEF)
ifeq ($(CROSS), )
ifeq ($(SYS_BIT), 64)
CFLAGS += -m32 
endif
endif


LIBDIR = -L ./
LIBDIR += -L $(OPENSSL_DIR)lib
LIBDIR += -L $(JSON_DIR)lib

LIBS = $(LIBDIR) -l$(PROJECT)
LIBS += -lpthread
LIBS += -lcrypto
LIBS += -lssl
LIBS += -ljson-c


ifeq ($(FILETYPE), cpp) 
COMPILE = $(CXX)
else
COMPILE = $(CC)
endif

COMPILE.$(FILETYPE) = $(COMPILE) $(CFLAGS) -c
LINK.$(FILETYPE) = $(COMPILE) $(CFLAGS)

#lib source
LIB_SRC = $(wildcard lib/*.$(FILETYPE))
LIB_OBJ = $(LIB_SRC:%.$(FILETYPE)=%.o)

#target source
SRC = $(wildcard *.$(FILETYPE))
SRC += $(wildcard src/*.$(FILETYPE))
OBJ = $(SRC:%.$(FILETYPE)=%.o)

#test source
TEST_SRC = $(wildcard test/*.$(FILETYPE))
TEST_OBJ = $(TEST_SRC:%.$(FILETYPE)=%.o)


#include source
INC = $(wildcard inc/*.h)

LIB_TARGET = lib$(PROJECT).so
TARGET = $(PROJECT)
TEST_TARGET = test_$(PROJECT)

.PHONY : clean all

all : lib test $(TARGET)
$(TARGET) : $(OBJ)
	$(LINK.$(FILETYPE)) -o $@ $^ $(LIBS)


lib : $(LIB_TARGET)
$(LIB_TARGET) : $(LIB_OBJ)
	$(LINK.$(FILETYPE)) -o $@ $^ -shared -fpic

test : $(TEST_TARGET)
$(TEST_TARGET) : $(TEST_OBJ)
	$(LINK.$(FILETYPE)) -o $@ $^ $(LIBS)
	

	
#$(OBJ): %.o : %.cpp
#	@$(COMPILE.cpp) -o $@ $^
	

clean:
	@rm -f $(LIB_TARGET)
	@rm -f $(LIB_OBJ)
	@rm -f $(TARGET)
	@rm -f $(OBJ)
	@rm -f $(TEST_TARGET)
	@rm -f $(TEST_OBJ)
	
cleanall:
	@make clean
	
install:
	@mkdir -p $(INSTALL_DIR)	
	@cp $(LIB_TARGET) inc/ycs.h $(INSTALL_DIR)

uninstall:
	@rm -rf $(INSTALL_DIR)$(LIB_TARGET) $(INSTALL_DIR)ycs.h

DATE=`date +%Y%m%d%H%M%S`
NAME=backup_$(DATE).tar.bz2
FILE=`ls | grep -v backup_`
backup:clean
	@echo $(DATE)
	@echo $(NAME)
#	tar cfvj $(NAME) inc lib src Makefile rmoldbackup.sh
	tar cfvj $(NAME) $(FILE)
	./rmoldbackup.sh backup
	



