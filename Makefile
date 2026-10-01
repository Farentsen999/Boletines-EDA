# =============================================================================
# Raiz del repositorio: corre las pruebas de AMBOS boletines.
#
#   make              boletin 1 + boletin 2 (completo; puede tardar ~1 hora)
#   make boletin-1    solo el boletin 1
#   make boletin-2    solo el boletin 2
#   make quick        prueba de humo de ambos (parametros minimos, pocos minutos)
#   make clean        borra binarios  |  make distclean   borra binarios y resultados
#
# Cualquier variable se propaga a los Makefile de cada boletin, p. ej.:
#   make RUNS=64
#   make PIN="taskset -c 2"
# Para ver las variables de cada uno:  make -C boletin-1 help   /   make -C boletin-2 help
#
# IMPORTANTE: no usar "make -j" (las mediciones de tiempo deben correr de a una).
# Consejo: durante la corrida completa, no usar el computador para nada mas.
# =============================================================================

.NOTPARALLEL:
.PHONY: all boletin-1 boletin-2 quick clean distclean help

all: boletin-1 boletin-2

boletin-1:
	$(MAKE) -C boletin-1 all

boletin-2:
	$(MAKE) -C boletin-2 all

quick:
	$(MAKE) -C boletin-1 quick
	$(MAKE) -C boletin-2 quick

clean:
	$(MAKE) -C boletin-1 clean
	$(MAKE) -C boletin-2 clean

distclean:
	$(MAKE) -C boletin-1 distclean
	$(MAKE) -C boletin-2 distclean

help:
	@sed -n '2,15p' Makefile