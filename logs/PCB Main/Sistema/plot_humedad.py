#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt


def find_col(df, contains: str) -> str:
    contains = contains.lower()
    for c in df.columns:
        if contains in str(c).lower():
            return c
    raise KeyError(f"No encuentro ninguna columna que contenga: {contains!r}. Columnas: {list(df.columns)}")


def to_numeric_series(s: pd.Series) -> pd.Series:
    if pd.api.types.is_numeric_dtype(s):
        return s
    return pd.to_numeric(
        s.astype(str).str.replace(",", ".", regex=False).str.strip(),
        errors="coerce"
    )


def main():
    parser = argparse.ArgumentParser(description="Grafica Humedad Aire vs Tiempo (2 s por ciclo) desde humedad.csv")
    parser.add_argument("csv_path", help="Ruta al CSV (ej: humedad.csv)")
    parser.add_argument("--out", default="", help="Si se indica, guarda la imagen (png/pdf) en esta ruta.")
    parser.add_argument("--show", action="store_true", help="Muestra la gráfica en pantalla.")
    args = parser.parse_args()

    df = pd.read_csv(args.csv_path)

    col_ciclo = find_col(df, "ciclo")
    col_hum = find_col(df, "hum")

    ciclo = to_numeric_series(df[col_ciclo])
    hum = to_numeric_series(df[col_hum])

    # Limpieza
    m = ciclo.notna() & hum.notna()
    ciclo = ciclo[m].astype(float)
    hum = hum[m].astype(float)

    # Tiempo en segundos: primer ciclo en t=0, y cada ciclo son 2 s
    ciclo0 = float(ciclo.iloc[0])
    t = (ciclo - ciclo0) * 2.0

    fig, ax = plt.subplots(figsize=(10, 5))

    ax.plot(t, hum, linewidth=2, color="blue", label="Hum Aire")

    # Ejes
    ax.set_ylabel("(%)")
    ax.set_ylim(0, 100)
    ax.grid(True, alpha=0.3)

    # Eje X con unidades
    ax.set_xlabel("(s)")

    # Bordes exactos
    ax.set_xlim(t.iloc[0], t.iloc[-1])
    ax.margins(x=0)

    # Ticks X: espaciado automático (máx ~20 labels)
    max_labels = 20
    n = len(t)
    step = max(1, int(np.ceil(n / max_labels)))
    ax.set_xticks(t.iloc[::step])

    # Rotación en diagonal
    plt.setp(ax.get_xticklabels(), rotation=45, ha="right")

    # Leyenda
    ax.legend(loc="best")

    plt.tight_layout()

    if args.out:
        plt.savefig(args.out, dpi=200, bbox_inches="tight")
        print(f"Guardado: {args.out}")

    if args.show or not args.out:
        plt.show()


if __name__ == "__main__":
    main()
