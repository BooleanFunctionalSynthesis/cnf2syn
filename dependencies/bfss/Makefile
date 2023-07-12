#optional arguments:
# 	UNIGEN=NO
# 	BUILD=RELEASE/DEBUG

UNIGEN=NO
BUILD=DEBUG
# Architecture required by ABC
CPP_FLAGS += -DLIN64

BFSS 	= bfss
RCNF  	= readCnf
ORDR  	= genVarOrder
VRFY  	= verify
RSUB    = revsub
#LIB_BFSS = libbfss.a
LIB_COMBFSS = libcombfss.a

#PREP    = prep

ABC_PATH = ../abc
SCALMC_PATH = ./dependencies/scalmc

ifndef CXX
CXX = g++
endif

SRCDIR   = src
OBJDIR   = obj
BINDIR   = bin

TARGET_RCNF  = $(BINDIR)/$(RCNF)
TARGET_BFSS  = $(BINDIR)/$(BFSS)
TARGET_ORDR  = $(BINDIR)/$(ORDR)
TARGET_VRFY  = $(BINDIR)/$(VRFY)
TARGET_RSUB  = $(BINDIR)/$(RSUB)
#TARGET_PREP  = $(BINDIR)/$(PREP)

ABC_INCLUDES = -I $(ABC_PATH) -I $(ABC_PATH)/src
UGEN_INCLUDES = -I $(SCALMC_PATH)/build/cmsat5-src/ -I $(SCALMC_PATH)/src/
LIB_DIRS = -L $(SCALMC_PATH)/build/lib/ -L $(ABC_PATH)/
DIR_INCLUDES = $(ABC_INCLUDES) $(UGEN_INCLUDES) $(LIB_DIRS)

LIB_UGEN   = -Wl,-Bdynamic -lcryptominisat5
LIB_ABC    = -Wl,-Bstatic  -labc
LIB_COMMON = -Wl,-Bdynamic -lm -ldl -lreadline -ltermcap -lpthread -fopenmp -lrt -Wl,-Bdynamic -lboost_program_options -Wl,-Bdynamic -lz

ifeq ($(UNIGEN), NO)
CPP_FLAGS += -std=c++11 -DNO_UNIGEN  
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_COMMON)
else
CPP_FLAGS += -std=c++11 
LFLAGS    = $(DIR_INCLUDES) $(LIB_ABC) $(LIB_UGEN) $(LIB_COMMON)
endif

ifeq ($(BUILD),DEBUG)
CPP_FLAGS += -O0 -g
else ifeq ($(BUILD),RELEASE)
CPP_FLAGS += -O3 -s -DNDEBUG
else
CPP_FLAGS += -O3
endif

COMMON_SOURCES  = $(SRCDIR)/nnf.cpp $(SRCDIR)/helper.cpp

BFSS_SOURCES  = $(SRCDIR)/bfss.cpp $(COMMON_SOURCES)
ORDR_SOURCES  = $(SRCDIR)/genVarOrder.cpp $(COMMON_SOURCES)
RCNF_SOURCES  = $(SRCDIR)/readCnf.cpp
VRFY_SOURCES  = $(SRCDIR)/verify.cpp 
RSUB_SOURCES  = $(SRCDIR)/revsub.cpp
#PREP_SOURCES  = $(SRCDIR)/preprocess.cpp $(SRCDIR)/readCnf.cpp $(COMMON_SOURCES)
BFSS_OBJECTS  = $(BFSS_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
ORDR_OBJECTS  = $(ORDR_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
RCNF_OBJECTS  = $(RCNF_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
VRFY_OBJECTS  = $(VRFY_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
RSUB_OBJECTS  = $(RSUB_SOURCES:$(SRCDIR)/%.cpp=$(OBJDIR)/%.o)
ALL_OBJECTS   = $(sort $(BFSS_OBJECTS) $(ORDR_OBJECTS) $(RCNF_OBJECTS) $(VRFY_OBJECTS) $(RSUB_OBJECTS))

.PHONY: all clean remove bfss readCnf genVarOrder verify revsub directories prep
all: bfss readCnf genVarOrder verify revsub prep
bfss: directories $(TARGET_BFSS)
genVarOrder: directories $(TARGET_ORDR)
readCnf: directories $(TARGET_RCNF)
verify: directories $(TARGET_VRFY)
revsub: directories $(TARGET_RSUB)
#prep: directories $(TARGET_PREP)

directories:
	@mkdir -p $(OBJDIR)
	@mkdir -p $(BINDIR)

$(TARGET_BFSS): $(BFSS_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - bfss"

$(TARGET_ORDR): $(ORDR_OBJECTS)
	$(CXX) $(CPP_FLAGS) -o $@ $^ $(LFLAGS)
	@echo "Built Target! - genVarOrder"

$(TARGET_RCNF): $(RCNF_SOURCES)
	$(CXX) $(CPP_FLAGS) -DMAIN $^ -o $@
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - readCnf"

$(TARGET_VRFY): $(VRFY_SOURCES)
	$(CXX) $(CPP_FLAGS) $^ -o $@ $(DIR_INCLUDES) $(LIB_ABC) $(LIB_COMMON)
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - verify"

$(TARGET_RSUB): $(RSUB_SOURCES)
	$(CXX) $^ -o $@
	@echo "Compiled "$^" successfully!"
	@echo "Built Target! - revsub"

#$(TARGET_PREP): $(PREP_SOURCES)
#	$(CXX) $(CPP_FLAGS) -DNO_UNIGEN $^ -o $@ $(DIR_INCLUDES) $(LIB_ABC) $(LIB_COMMON)
#	@echo "Compiled "$^" successfully!"
#	@echo "Built Target! - prep"

$(ALL_OBJECTS): $(OBJDIR)/%.o : $(SRCDIR)/%.cpp
	$(CXX) $(CPP_FLAGS) -c $^ -o $@  $(LFLAGS)
	@echo "Compiled "$<" successfully!"

#$(LIB_BFSS): $(ALL_OBJECTS)
#	ar rcs $(LIB_BFSS) $(ALL_OBJECTS)
#	ranlib $(LIB_BFSS) 

$(LIB_BFSS): obj/helper.o obj/nnf.o obj/readCnf.o obj/bfss.o obj/revsub.o obj/verify.o
	ar rcs $@ $^ 
	ranlib $@ 
    
$(LIB_COMBFSS): obj/helper.o obj/nnf.o obj/readCnf.o
	ar rcs $@ $^ 
	ranlib $@ 
clean:
	@$(RM) $(BFSS_OBJECTS)
	@echo "Cleanup complete!"

remove: clean
	@$(RM) $(TARGET_BFSS) $(TARGET_ORDR) $(TARGET_RCNF) $(TARGET_VRFY) $(TARGET_RSUB) $(TARGET_PREP)
	@echo "Executable removed!"
