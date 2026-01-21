# Configuration
PROJECT_NAME = main
BUILD_DIR = build
SRC_DIR = src
INCLUDE_DIR = include
TRACES_DIR = traces

# Chemins FreeRTOS
FREERTOS_ROOT = C:/Users/bensi/WorkSpace_hadi/FreeRTOSv202411.00
FREERTOS_SRC = $(FREERTOS_ROOT)/FreeRTOS/Source
FREERTOS_PORT = $(FREERTOS_SRC)/portable/MSVC-MingW
FREERTOS_MEMMANG = $(FREERTOS_SRC)/portable/MemMang

# Compilateur et flags
CC = cl.exe
CFLAGS = /nologo /Zi /EHsc /MD /Fd$(BUILD_DIR)/ /Fo$(BUILD_DIR)/
INCLUDES = /I$(INCLUDE_DIR) \
           /I$(FREERTOS_SRC)/include \
           /I$(FREERTOS_PORT)

LDFLAGS = /link /machine:x64 /OUT:$(BUILD_DIR)/$(PROJECT_NAME).exe kernel32.lib winmm.lib

# Fichiers source
# Fichiers source
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/scheduler.c \
          $(FREERTOS_SRC)/tasks.c \
          $(FREERTOS_SRC)/queue.c \
          $(FREERTOS_SRC)/timers.c \
          $(FREERTOS_SRC)/list.c \
          $(FREERTOS_MEMMANG)/heap_3.c \
          $(FREERTOS_PORT)/port.c


# Cible par défaut
all: $(BUILD_DIR)/$(PROJECT_NAME).exe

# Compilation
$(BUILD_DIR)/$(PROJECT_NAME).exe: $(SOURCES)
	@if not exist $(BUILD_DIR) mkdir $(BUILD_DIR)
	$(CC) $(CFLAGS) $(INCLUDES) $(SOURCES) $(LDFLAGS)

# Exécution
run: all
	@echo.
	@echo === Execution ===
	@$(BUILD_DIR)\$(PROJECT_NAME).exe

# Nettoyage
clean:
	@if exist $(BUILD_DIR)\*.obj del /Q $(BUILD_DIR)\*.obj
	@if exist $(BUILD_DIR)\*.exe del /Q $(BUILD_DIR)\*.exe
	@if exist $(BUILD_DIR)\*.pdb del /Q $(BUILD_DIR)\*.pdb
	@if exist $(BUILD_DIR)\*.ilk del /Q $(BUILD_DIR)\*.ilk
	@if exist $(TRACES_DIR)\*.pdf del /Q $(TRACES_DIR)\*.pdf
	@if exist $(TRACES_DIR)\*.png del /Q $(TRACES_DIR)\*.png

	@echo Build directory cleaned!


# Nettoyage complet
distclean: clean
	@if exist $(BUILD_DIR) rmdir /Q $(BUILD_DIR)
	@echo Project cleaned completely!

# Aide
help:
	@echo Commandes disponibles:
	@echo   make          - Compile le projet
	@echo   make run      - Compile et execute
	@echo   make clean    - Nettoie les fichiers compiles
	@echo   make distclean - Nettoie tout
	@echo   make help     - Affiche cette aide

.PHONY: all run clean distclean help
