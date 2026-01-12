#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
from datetime import datetime, date, time as dtime
import re

import numpy as np
import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.dates as mdates


def pick_col(columns, must_contain_tokens):
    tokens = [t.lower() for t in must_contain_tokens]
    for c in columns:
        name = str(c).lower()
        if all(t in name for t in tokens):
            return c
    return None


def to_numeric_clean(s: pd.Series) -> pd.Series:
    def parse_one(x):
        if pd.isna(x):
            return np.nan
        txt = str(x).strip().replace(",", ".")
        txt = re.sub(r"[^0-9\.\-]+", "", txt)  # deja 0-9 . -
        if txt in ("", ".", "-", "-.", ".-"):
            return np.nan
        return float(txt)

    return s.map(parse_one)


def main():
    ap = argparse.ArgumentParser(description="Grafica Humedad y Temperatura del terreno con eje X en hora del día.")
    ap.add_argument("-i", "--input", default="hum_temp_terreno.csv", help="CSV de entrada")
    ap.add_argument("--start", default="09:10", help="Hora de la primera muestra HH:MM (default 09:10)")
    ap.add_argument("--dt", type=float, default=10.0, help="Periodo de muestreo en segundos (default 10)")
    ap.add_argument("-o", "--out", default="hum_temp_terreno.png", help="Imagen de salida")
    ap.add_argument("--show", action="store_true", help="Mostrar gráfica en pantalla")
    args = ap.parse_args()

    df = pd.read_csv(args.input, sep=None, engine="python")

    col_hum = pick_col(df.columns, ["hum", "suelo"]) or pick_col(df.columns, ["hum"])
    col_tmp = pick_col(df.columns, ["temp", "suelo"]) or pick_col(df.columns, ["temp"])

    if col_hum is None or col_tmp is None:
        raise ValueError(
            f"No he podido detectar columnas de Hum/Temp. Columnas: {list(df.columns)}"
        )

    df[col_hum] = to_numeric_clean(df[col_hum])
    df[col_tmp] = to_numeric_clean(df[col_tmp])
    df = df.dropna(subset=[col_hum, col_tmp]).reset_index(drop=True)

    if len(df) < 2:
        raise ValueError("No hay suficientes muestras válidas para graficar.")

    # Tiempo: hoy + 09:10 + n*10s (solo para mostrar HH:MM)
    hh, mm = map(int, args.start.split(":"))
    start_dt = datetime.combine(date.today(), dtime(hour=hh, minute=mm, second=0))
    t = start_dt + pd.to_timedelta(np.arange(len(df)) * args.dt, unit="s")

    fig, ax_h = plt.subplots(figsize=(12, 5))

    # Humedad primero (azul) en eje izquierdo
    lh, = ax_h.plot(t, df[col_hum], linewidth=1.8, color="blue", label="Hum Suelo")
    ax_h.set_ylabel("(%)")
    ax_h.set_ylim(0, 100)
    ax_h.grid(True, alpha=0.35)
    ax_h.set_xlabel("")

    # Temperatura después (rojo/naranja) en eje derecho
    ax_t = ax_h.twinx()
    lt, = ax_t.plot(t, df[col_tmp], linewidth=1.8, color="orangered", label="Temp Suelo")
    ax_t.set_ylabel("°C")
    ax_t.set_ylim(0, 40)

    # Eje X: HH:MM
    ax_h.xaxis.set_major_locator(mdates.AutoDateLocator(minticks=6, maxticks=12))
    ax_h.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
    plt.setp(ax_h.get_xticklabels(), rotation=45, ha="right")

    # Bordes exactos
    ax_h.set_xlim(t[0], t[-1])
    ax_h.margins(x=0)
    ax_t.margins(x=0)

    # Leyenda: primero humedad, luego temp
    ax_h.legend([lh, lt], ["Hum Suelo", "Temp Suelo"], loc="best")

    plt.tight_layout()
    plt.savefig(args.out, dpi=200, bbox_inches="tight")
    print(f"Guardado: {args.out}")

    if args.show:
        plt.show()


if __name__ == "__main__":
    main()
