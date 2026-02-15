# =========================================================
# Configuration
# =========================================================
PROJECT_NAME = main
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
TRACES_DIR = traces

# Python
PYTHON = python
TRACE_SCRIPT = trace_to_gantt.py

# =========================================================
# FreeRTOS paths
# =========================================================
FREERTOS_ROOT = C:/Users/bensi/WorkSpace_hadi/FreeRTOSv202411.00
FREERTOS_SRC = $(FREERTOS_ROOT)/FreeRTOS/Source
FREERTOS_PORT = $(FREERTOS_SRC)/portable/MSVC-MingW
FREERTOS_MEMMANG = $(FREERTOS_SRC)/portable/MemMang

# =========================================================
# Compiler & flags
# =========================================================
CC = cl.exe
CFLAGS = /nologo /Zi /EHsc /MD /Fd$(BUILD_DIR)/ /Fo$(BUILD_DIR)/
INCLUDES = /I$(INCLUDE_DIR) \
           /I$(FREERTOS_SRC)/include \
           /I$(FREERTOS_PORT)

LDFLAGS = /link /machine:x64 /OUT:$(BUILD_DIR)/$(PROJECT_NAME).exe kernel32.lib winmm.lib

# =========================================================
# Sources
# =========================================================
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/scheduler.c \
          $(FREERTOS_SRC)/tasks.c \
          $(FREERTOS_SRC)/queue.c \
          $(FREERTOS_SRC)/timers.c \
          $(FREERTOS_SRC)/list.c \
          $(FREERTOS_MEMMANG)/heap_1.c \
          $(FREERTOS_PORT)/port.c

# =========================================================
# Default target
# =========================================================
all: $(BUILD_DIR)/$(PROJECT_NAME).exe

# =========================================================
# Build
# =========================================================
$(BUILD_DIR)/$(PROJECT_NAME).exe: $(SOURCES)
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) $(LDFLAGS)

# =========================================================
# Run
# =========================================================
run: all
	@echo.
	@echo === Execution ===
	@$(BUILD_DIR)\$(PROJECT_NAME).exe

# =========================================================
# Trace: run + log + python visualization
# =========================================================
trace: 
	@echo tracing...
	@$(PYTHON) traces/trace_to_gantt.py

# =========================================================
# Clean traces only
# =========================================================
clean-trace:
	@if exist $(TRACES_DIR)\*.pdf del /Q $(TRACES_DIR)\*.pdf
	@if exist $(TRACES_DIR)\*.png del /Q $(TRACES_DIR)\*.png
	@echo Traces cleaned!

# =========================================================
# Clean build + traces
# =========================================================
clean: clean-trace
	@if exist $(BUILD_DIR)\*.obj del /Q $(BUILD_DIR)\*.obj
	@if exist $(BUILD_DIR)\*.exe del /Q $(BUILD_DIR)\*.exe
	@if exist $(BUILD_DIR)\*.pdb del /Q $(BUILD_DIR)\*.pdb
	@if exist $(BUILD_DIR)\*.ilk del /Q $(BUILD_DIR)\*.ilk
	@echo Build cleaned!

# =========================================================
# Full clean
# =========================================================
distclean: clean
	@if exist $(BUILD_DIR) rmdir /Q /S $(BUILD_DIR)
	@if exist $(TRACES_DIR) rmdir /Q /S $(TRACES_DIR)
	@echo Project fully cleaned!

# =========================================================
# Help
# =========================================================
help:
	@echo Available commands:
	@echo   make             - Compile project
	@echo   make run         - Compile and execute
	@echo   make trace       - Generate scheduling trace (PDF/PNG)
	@echo   make clean       - Clean build and traces
	@echo   make clean-trace - Clean traces only
	@echo   make distclean   - Full cleanup
	@echo   make help        - Show this help

.PHONY: all run trace clean clean-trace distclean help
