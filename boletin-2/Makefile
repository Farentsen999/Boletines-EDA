# =============================================================================
# Boletin 2 - Arboles de busqueda (AVL, Rojo-Negro = std::set, Splay)
#
#   make            compila y corre TODOS los experimentos del boletin
#   make search     search vs n (consultas uniformes: linea base)
#   make insert     insert vs n
#   make delete     delete vs n
#   make skew       search con distribuciones no uniformes (claves frecuentes vs poco frecuentes)
#   make repeat     k-esimo acceso consecutivo a una misma clave poco buscada (splay)
#   make control    control: misma clave repetida vs claves distintas
#   make quick      prueba de humo (parametros minimos, ~1-2 min, sin valor estadistico)
#   make clean      borra binarios      |  make distclean   borra binarios y resultados
#
# Los parametros se pueden sobreescribir, p. ej.:
#   make RUNS=64 N_SKEW=500000
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
RUNS ?= 32
SEED ?= 1

# --- search / insert / delete vs n (n = N_LOW * STEP^k) ----------------------
# 2^10 .. 2^20 (11 puntos): de "el arbol cabe en cache" a "no cabe", donde el
# costo de los fallos de cache empieza a dominar sobre la altura del arbol.
N_LOW  ?= 1024
N_HIGH ?= 1048576
STEP   ?= 2
# consultas por lote (search)   [mut-ops: automatico = n/10 acotado a 100..100000]
OPS    ?= 100000

# --- skew / repeat / control (n fijo) ----------------------------------------
# n = 10^6: el arbol no cabe en cache, asi que la ventaja de splay sobre las
# claves frecuentes (que quedan cerca de la raiz y en cache) se puede ver.
# Requisito del programa: N_SKEW >= 20 * HOT.
N_SKEW   ?= 1000000
# escala de las distribuciones (media/parametro de las claves frecuentes)
HOT      ?= 1000
# consultas a claves poco frecuentes por lote
COLD_OPS ?= 10000
# repeat: claves frias distintas por repeticion (mas claves = menos ruido)
KEYS     ?= 100
# repeat: accesos consecutivos a cada clave fria
REPEATS  ?= 20
DISTS    ?= zipf,geometric,poisson,binomial,negbinomial

EXPS := search insert delete skew repeat control

COMMON_ARGS := --runs $(RUNS) --lower $(N_LOW) --upper $(N_HIGH) --step $(STEP) \
               --ops $(OPS) --n $(N_SKEW) --hot $(HOT) --cold-ops $(COLD_OPS) \
               --keys $(KEYS) --repeats $(REPEATS) --dist $(DISTS) --seed $(SEED)

HDR := $(wildcard include/*.hpp)

CSV := $(addprefix $(OUT)/res_trees_,$(addsuffix .csv,$(EXPS)))

.NOTPARALLEL:
.PHONY: all $(EXPS) quick clean distclean help

all: $(EXPS)

$(EXPS): %: $(OUT)/res_trees_%.csv

help:
	@sed -n '2,20p' Makefile

# --- Compilacion --------------------------------------------------------------
$(BIN) $(OUT):
	mkdir -p $@

$(BIN)/bench_trees: src/bench_trees.cpp $(HDR) | $(BIN)
	$(CXX) $(CXXFLAGS) -o $@ $<

# --- Ejecucion (se escribe a .tmp y se renombra: un CSV a medias nunca cuenta) -
$(OUT)/res_trees_%.csv: $(BIN)/bench_trees | $(OUT)
	$(PIN) $(BIN)/bench_trees $* $@.tmp $(COMMON_ARGS) && mv $@.tmp $@

# --- Prueba de humo -------------------------------------------------------------
quick:
	$(MAKE) all OUT=resultados_quick RUNS=4 N_HIGH=8192 OPS=5000 N_SKEW=20000 HOT=500 \
	        COLD_OPS=1000 KEYS=10 REPEATS=8

clean:
	rm -rf $(BIN)

distclean: clean
	rm -rf resultados resultados_quick