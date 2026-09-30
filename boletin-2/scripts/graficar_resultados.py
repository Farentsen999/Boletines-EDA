#!/usr/bin/env python3
import glob
import os
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd
import seaborn as sns

# Configuración global de estilos
sns.set_theme(style="whitegrid")
plt.rcParams.update({"font.size": 11, "figure.autolayout": True})


def graficar_experimento_1(directorio_boletin1):
    """Grafica el Tiempo vs N para el Experimento 1 de Búsquedas."""
    archivos = glob.glob(os.path.join(directorio_boletin1, "res_exp1_*.csv"))
    if not archivos:
        print("[-] No se encontraron archivos res_exp1_*.csv en", directorio_boletin1)
        return

    dfs = []
    for f in archivos:
        try:
            df = pd.read_csv(f)
            dfs.append(df)
        except Exception as e:
            print(f"Error leyendo {f}: {e}")

    if not dfs:
        return

    data = pd.concat(dfs, ignore_index=True)

    plt.figure(figsize=(10, 6))
    for algo, df_algo in data.groupby("algoritmo"):
        df_algo = df_algo.sort_values("n")
        plt.plot(df_algo["n"], df_algo["t_mean"], marker="o", label=f"Algo: {algo}")
        plt.fill_between(
            df_algo["n"],
            df_algo["t_mean"] - df_algo["t_stdev"],
            df_algo["t_mean"] + df_algo["t_stdev"],
            alpha=0.15,
        )

    plt.xscale("log")
    plt.yscale("log")
    plt.xlabel("Tamaño de Secuencia (n)")
    plt.ylabel("Tiempo Promedio (ns)")
    plt.title("Experimento 1: Desempeño de Algoritmos de Búsqueda vs Tamaño n")
    plt.legend()
    plt.tight_layout()
    plt.savefig(
        os.path.join(directorio_boletin1, "grafico_exp1_busquedas.png"), dpi=300
    )
    plt.close()
    print(
        f"[+] Gráfico guardado: {os.path.join(directorio_boletin1, 'grafico_exp1_busquedas.png')}"
    )


def graficar_experimento_2(directorio_boletin1):
    """Grafica el Tiempo vs Posición para el Experimento 2 de Búsquedas."""
    archivos = glob.glob(os.path.join(directorio_boletin1, "res_exp2_*.csv"))
    if not archivos:
        print("[-] No se encontraron archivos res_exp2_*.csv")
        return

    dfs = []
    for f in archivos:
        try:
            dfs.append(pd.read_csv(f))
        except Exception as e:
            print(f"Error leyendo {f}: {e}")

    if not dfs:
        return

    data = pd.concat(dfs, ignore_index=True)

    plt.figure(figsize=(10, 6))
    sns.barplot(
        data=data,
        x="posicion",
        y="t_mean",
        hue="algoritmo",
        errorbar=None,
        capsize=0.1,
    )
    plt.xlabel("Posición del Elemento")
    plt.ylabel("Tiempo Promedio (ns)")
    plt.title("Experimento 2: Impacto de la Posición del Elemento en Búsquedas")
    plt.yscale("log")
    plt.legend(title="Algoritmo")
    plt.tight_layout()
    plt.savefig(
        os.path.join(directorio_boletin1, "grafico_exp2_posicion.png"), dpi=300
    )
    plt.close()
    print(
        f"[+] Gráfico guardado: {os.path.join(directorio_boletin1, 'grafico_exp2_posicion.png')}"
    )


def graficar_heaps(directorio_boletin1):
    """Grafica la comparativa entre Binary Heap y Binomial Heap por Operación."""
    archivos = glob.glob(os.path.join(directorio_boletin1, "res_heap_*.csv"))
    if not archivos:
        print("[-] No se encontraron archivos res_heap_*.csv")
        return

    dfs = [pd.read_csv(f) for f in archivos]
    data = pd.concat(dfs, ignore_index=True)

    operaciones = data["operacion"].unique()

    fig, axes = plt.subplots(
        2, 2, figsize=(12, 10), sharex=True
    )
    axes = axes.flatten()

    for idx, op in enumerate(operaciones):
        ax = axes[idx]
        df_op = data[data["operacion"] == op]

        for heap_type, df_heap in df_op.groupby("heap"):
            df_heap = df_heap.sort_values("n")
            ax.plot(
                df_heap["n"],
                df_heap["t_mean"],
                marker="o",
                label=f"{heap_type} heap",
            )
            ax.fill_between(
                df_heap["n"],
                df_heap["t_mean"] - df_heap["t_stdev"],
                df_heap["t_mean"] + df_heap["t_stdev"],
                alpha=0.15,
            )

        ax.set_xscale("log")
        ax.set_yscale("log")
        ax.set_title(f"Operación: {op}")
        ax.set_xlabel("n")
        ax.set_ylabel("Tiempo Promedio (ns)")
        ax.legend()

    plt.suptitle("Benchmark de Heaps: Binario vs Binomial", fontsize=14)
    plt.tight_layout()
    plt.savefig(
        os.path.join(directorio_boletin1, "grafico_heaps_benchmark.png"),
        dpi=300,
    )
    plt.close()
    print(
        f"[+] Gráfico guardado: {os.path.join(directorio_boletin1, 'grafico_heaps_benchmark.png')}"
    )


def graficar_arboles(directorio_boletin2):
    """Grafica los resultados del Boletín 2 (Árboles AVL, Red-Black, Splay)."""
    archivo_trees = os.path.join(directorio_boletin2, "res_trees.csv")
    if not os.path.exists(archivo_trees):
        print(f"[-] No existe el archivo {archivo_trees}")
        return

    data = pd.read_csv(archivo_trees)

    plt.figure(figsize=(10, 6))
    for arbol, df_arbol in data.groupby("arbol"):
        df_arbol = df_arbol.sort_values("n")
        plt.plot(
            df_arbol["n"], df_arbol["t_mean"], marker="s", label=f"Árbol: {arbol}"
        )
        plt.fill_between(
            df_arbol["n"],
            df_arbol["t_mean"] - df_arbol["t_stdev"],
            df_arbol["t_mean"] + df_arbol["t_stdev"],
            alpha=0.15,
        )

    plt.xscale("log")
    plt.yscale("log")
    plt.xlabel("Número de Elementos (n)")
    plt.ylabel("Tiempo Promedio (ns)")
    plt.title("Boletín 2: Comparativa de Árboles de Búsqueda Auto-balanceados")
    plt.legend()
    plt.tight_layout()
    plt.savefig(
        os.path.join(directorio_boletin2, "grafico_arboles_benchmark.png"),
        dpi=300,
    )
    plt.close()
    print(
        f"[+] Gráfico guardado: {os.path.join(directorio_boletin2, 'grafico_arboles_benchmark.png')}"
    )


if __name__ == "__main__":
    b1_dir = "boletin-1"
    b2_dir = "boletin-2"

    print("--- Generando Gráficos para Boletín 1 ---")
    graficar_experimento_1(b1_dir)
    graficar_experimento_2(b1_dir)
    graficar_heaps(b1_dir)

    print("\n--- Generando Gráficos para Boletín 2 ---")
    graficar_arboles(b2_dir)