CXXFLAGS	+= -I./sources/app/sdcard

VPATH += sources/app/sdcard

OBJ += $(OBJ_DIR)/SDCard.o
OBJ += $(OBJ_DIR)/recorder.o
OBJ += $(OBJ_DIR)/utilities.o