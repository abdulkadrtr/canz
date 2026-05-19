CC      := gcc
CFLAGS  := -Wall -O3 -std=c11

CANZ_DIR := ../canz
INCLUDES := -I$(CANZ_DIR)/include
LDFLAGS  := -L$(CANZ_DIR)/build/lib -lcanz -lzstd

RPATH    := -Wl,-rpath,$(realpath $(CANZ_DIR)/build/lib)

TARGET := canz_app

$(TARGET): main.c
	$(CC) $(CFLAGS) $(INCLUDES) $< -o $@ $(LDFLAGS) $(RPATH)
	@echo "Build completed: ./$(TARGET)"

clean:
	rm -f $(TARGET)

.PHONY: clean
