# =============================================================================
# Boletin 1 - Busqueda y colas de prioridad
#
#   make            compila y corre TODOS los experimentos del boletin
#   make exp1       busquedas: tiempo vs tamano de la secuencia (6 algoritmos)
#   make exp2       busquedas: tiempo vs posicion del elemento   (6 algoritmos)
#   make heaps      heaps binario/binomial: insert, top, pop, meld, build
#   make heaps-shapes   top y meld del heap binomial con otras formas (pow2/random)
#   make quick      prueba de humo (parametros minimos, ~1-2 min, sin valor estadistico)
#   make clean      borra binarios      |  make distclean   borra binarios y resultados
#
# Los parametros se pueden sobreescribir, p. ej.:
#   make RUNS=64 N_HIGH=4194304
#   make PIN="taskset -c 2"        (fija el proceso a un nucleo: mediciones mas estables)
#
# IMPORTANTE: no usar "make -j". Las mediciones de tiempo deben correr de a una.
# Los CSV se generan en $(OUT)/ y un experimento ya terminado no se repite
# (para repetirlo, borrar su CSV o hacer "make distclean").
# =============================================================================

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2
PIN      ?=

BIN := bin
OUT ?= resultados

# --- Repeticiones y semilla --------------------------------------------------
# repeticiones por punto (el boletin exige >= 32)
RUNS      ?= 32
# exp2 mide busquedas individuales (muy cortas): mas repeticiones
RUNS_EXP2 ?= 200
SEED      ?= 1

# --- Experimento 1: tiempo vs n (n = N_LOW * STEP^k) -------------------------
# 2^10 .. 2^24 con paso geometrico 2 = 15 puntos: sirve para ejes log-log y
# cubre desde "todo en cache L1/L2" hasta "memoria principal" (64 MB de ints).
N_LOW  ?= 1024
N_HIGH ?= 16777216
STEP   ?= 2

# --- Experimento 2: tiempo vs posicion, con n fijo ---------------------------
# n = 10^6 (como en el informe). El programa barre 40 posiciones: 20 en escala
# logaritmica (0..10^4, donde se ve la ventaja de la busqueda galopante) y 20
# lineales hasta el final del arreglo (donde se ve la diferencia secuencial/binaria).
N_FIXED ?= 1000000

# --- Heaps: tamano n de 2^10 a 2^20 (potencias de 2 => "worst" es exacto) -----
HEAP_LOW  ?= 1024
HEAP_HIGH ?= 1048576
HEAP_STEP ?= 2
# llamadas a top() por lote
HEAP_OPS_TOP ?= 100000

ALGOS    := sec bin gal stl_find stl_bin stl_lower
HEAP_OPS := insert top pop meld build

HEAP_ARGS := --runs $(RUNS) --lower $(HEAP_LOW) --upper $(HEAP_HIGH) \
             --step $(HEAP_STEP) --ops $(HEAP_OPS_TOP) --seed $(SEED)

HDR := $(wildcard include/*.hpp)

EXP1_CSV  := $(addprefix $(OUT)/res_exp1_,$(addsuffix .csv,$(ALGOS)))
EXP2_CSV  := $(addprefix $(OUT)/res_exp2_,$(addsuffix .csv,$(ALGOS)))
HEAP_CSV  := $(addprefix $(OUT)/res_heap_,$(addsuffix .csv,$(HEAP_OPS)))
SHAPE_CSV := $(foreach s,pow2 random,$(foreach o,top meld,$(OUT)/res_heapshape_$(s)_$(o).csv))

.NOTPARALLEL:
.PHONY: all exp1 exp2 heaps heaps-shapes quick clean distclean help

all: exp1 exp2 heaps heaps-shapes

exp1:          $(EXP1_CSV)
exp2:          $(EXP2_CSV)
heaps:         $(HEAP_CSV)
heaps-shapes:  $(SHAPE_CSV)

help:
	@sed -n '2,19p' Makefile

# --- Compilacion --------------------------------------------------------------
$(BIN) $(OUT):
	mkdir -p $@

$(BIN)/exp1: src/searches1.cpp $(HDR) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/exp2: src/searches2.cpp $(HDR) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

$(BIN)/bench_heaps: src/bench_heaps.cpp $(HDR) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

# --- Ejecucion (se escribe a .tmp y se renombra: un CSV a medias nunca cuenta) -
# exp1: <salida> <RUNS> <LOWER> <UPPER> <STEP> <ALGORITMO>
$(OUT)/res_exp1_%.csv: $(BIN)/exp1 | $(OUT)
	$(PIN) $(BIN)/exp1 $@.tmp $(RUNS) $(N_LOW) $(N_HIGH) $(STEP) $* && mv $@.tmp $@

# exp2: <salida> <RUNS> <N_FIJO> <UPPER (>= N)> <STEP (ignorado)> <ALGORITMO>
$(OUT)/res_exp2_%.csv: $(BIN)/exp2 | $(OUT)
	$(PIN) $(BIN)/exp2 $@.tmp $(RUNS_EXP2) $(N_FIXED) $(N_FIXED) 2 $* && mv $@.tmp $@

$(OUT)/res_heap_%.csv: $(BIN)/bench_heaps | $(OUT)
	$(PIN) $(BIN)/bench_heaps $* $@.tmp $(HEAP_ARGS) && mv $@.tmp $@

# Forma del heap binomial: pow2 = un solo arbol (top/meld ~ O(1)),
# random = numero tipico de raices (~log2(n)/2); "worst" ya esta en res_heap_*.
define SHAPE_RULE
$(OUT)/res_heapshape_$(1)_$(2).csv: $(BIN)/bench_heaps | $(OUT)
	$$(PIN) $$(BIN)/bench_heaps $(2) $$@.tmp $$(HEAP_ARGS) --shape $(1) && mv $$@.tmp $$@
endef
$(foreach s,pow2 random,$(foreach o,top meld,$(eval $(call SHAPE_RULE,$(s),$(o)))))

# --- Prueba de humo -------------------------------------------------------------
quick:
	$(MAKE) all OUT=resultados_quick RUNS=4 RUNS_EXP2=8 N_HIGH=16384 N_FIXED=20000 \
	        HEAP_HIGH=4096 HEAP_OPS_TOP=2000

clean:
	rm -rf $(BIN)

distclean: clean
	rm -rf resultados resultados_quick