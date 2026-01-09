#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


def find_col(df, contains: str) -> str:
    """Devuelve la primera columna que contenga el texto `contains` (case-insensitive)."""
    contains = contains.lower()
    for c in df.columns:
        if contains in str(c).lower():
            return c
    raise KeyError(f"No encuentro ninguna columna que contenga: {contains!r}. Columnas: {list(df.columns)}")


def to_numeric_series(s: pd.Series) -> pd.Series:
    """Convierte una serie a numérico soportando decimales con coma o punto."""
    if pd.api.types.is_numeric_dtype(s):
        return s
    return pd.to_numeric(
        s.astype(str).str.replace(",", ".", regex=False).str.strip(),
        errors="coerce"
    )


def main():
    parser = argparse.ArgumentParser(description="Grafica Temp Aire/Suelo y Humedad Aire desde un CSV.")
    parser.add_argument("csv_path", help="Ruta al archivo CSV (ej: sensores.csv)")
    parser.add_argument("--out", default="", help="Si se indica, guarda la imagen (png/pdf) en esta ruta.")
    parser.add_argument("--show", action="store_true", help="Muestra la gráfica en pantalla.")
    args = parser.parse_args()

    df = pd.read_csv(args.csv_path)

    # Columnas
    col_fecha = find_col(df, "fecha")
    col_hora = find_col(df, "hora")
    col_taire = find_col(df, "temp aire")   # SHT31
    col_tsuelo = find_col(df, "temp suelo") # DS18B20
    col_haire = find_col(df, "hum aire")    # SHT31

    # Datetime combinando Fecha + Hora
    dt = pd.to_datetime(
        df[col_fecha].astype(str).str.strip() + " " + df[col_hora].astype(str).str.strip(),
        dayfirst=True,
        errors="coerce"
    )

    df = df.assign(_dt=dt).dropna(subset=["_dt"]).sort_values("_dt")

    t_aire = to_numeric_series(df[col_taire])
    t_suelo = to_numeric_series(df[col_tsuelo])
    h_aire = to_numeric_series(df[col_haire])

    fig, ax1 = plt.subplots(figsize=(10, 5))

    # Temperaturas: aire naranja continua, suelo naranja discontinua
    temp_color = "orange"
    l1, = ax1.plot(df["_dt"], t_aire, linewidth=2, color=temp_color, linestyle="-",  label="Temp (SHT31)")
    l2, = ax1.plot(df["_dt"], t_suelo, linewidth=2, color=temp_color, linestyle="--", label="Temp (DS18B20)")

    # Solo unidades en eje Y izquierdo + rango 15-25 (aprox)
    ax1.set_ylabel("(°C)")
    ax1.set_ylim(15, 25)
    ax1.grid(True, alpha=0.3)

    # Sin etiqueta del eje X
    ax1.set_xlabel("")

    # Humedad: azul (eje derecho)
    ax2 = ax1.twinx()
    l3, = ax2.plot(df["_dt"], h_aire, linewidth=2, color="blue", linestyle="-", label="Hum (SHT31)")

    # Solo unidades en eje Y derecho + escala fija 0-100
    ax2.set_ylabel("(%)")
    ax2.set_ylim(0, 100)

    # Eje X: ticks EXACTOS del log, alternando (una sí, una no) y formato HH:MM
    xt = pd.to_datetime(df["_dt"]).dropna().unique()
    ax1.set_xticks(xt[::2])
    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
    plt.setp(ax1.get_xticklabels(), rotation=45, ha="right")

    # Que el gráfico empiece y acabe justo en los bordes
    ax1.set_xlim(df["_dt"].iloc[0], df["_dt"].iloc[-1])
    ax1.margins(x=0)

    # Leyenda con los textos pedidos
    ax1.legend([l1, l2, l3], [l1.get_label(), l2.get_label(), l3.get_label()], loc="best")

    # Sin título
    plt.tight_layout()

    if args.out:
        plt.savefig(args.out, dpi=200, bbox_inches="tight")
        print(f"Guardado: {args.out}")

    if args.show or not args.out:
        plt.show()


if __name__ == "__main__":
    main()
