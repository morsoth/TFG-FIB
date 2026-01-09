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


def strip_units(label: str) -> str:
    """
    Quita cosas típicas de unidades en el texto para la leyenda.
    Ej: "Batería (V)" -> "Batería", "Irradiancia W/m2" -> "Irradiancia"
    """
    s = str(label)
    for token in ["(V)", "(v)", "V", "v", "(W/m2)", "(W/m^2)", "W/m2", "W/m^2", "W/m", "w/m"]:
        s = s.replace(token, "")
    # Limpieza básica de paréntesis sobrantes y espacios
    s = s.replace("()", "").strip()
    while "  " in s:
        s = s.replace("  ", " ")
    return s.strip(" -_:")


def main():
    parser = argparse.ArgumentParser(description="Grafica Batería e Irradiancia desde un CSV.")
    parser.add_argument("csv_path", help="Ruta al archivo CSV (ej: autonomia.csv)")
    parser.add_argument("--out", default="", help="Si se indica, guarda la imagen (png/pdf) en esta ruta.")
    parser.add_argument("--show", action="store_true", help="Muestra la gráfica en pantalla.")
    args = parser.parse_args()

    df = pd.read_csv(args.csv_path)

    col_fecha = find_col(df, "fecha")
    col_hora = find_col(df, "hora")
    col_bat = find_col(df, "bateria")
    col_irr = find_col(df, "irradiancia")

    # Datetime combinando Fecha + Hora
    dt = pd.to_datetime(
        df[col_fecha].astype(str).str.strip() + " " + df[col_hora].astype(str).str.strip(),
        dayfirst=True,
        errors="coerce"
    )

    df = df.assign(_dt=dt).dropna(subset=["_dt"]).sort_values("_dt")

    bat = to_numeric_series(df[col_bat])
    irr = to_numeric_series(df[col_irr])

    fig, ax1 = plt.subplots(figsize=(10, 5))

    # Leyenda sin unidades (solo nombres)
    bat_legend = strip_units(col_bat)
    irr_legend = strip_units(col_irr)

    # Batería (rojo)
    bat_color = "red"
    l1, = ax1.plot(df["_dt"], bat, linewidth=2, color=bat_color, label=bat_legend)

    # Solo unidades en ejes Y
    ax1.set_ylabel("(V)")
    ax1.grid(True, alpha=0.3)

    # Sin etiqueta del eje X ("Hora")
    ax1.set_xlabel("")

    # Eje Y batería: 3.6–4.12 con margen
    ymin, ymax = 3.6, 4.12
    margin = 0.03
    ax1.set_ylim(ymin - margin, ymax + margin)

    # Irradiancia (línea continua) en 2º eje
    ax2 = ax1.twinx()
    l2, = ax2.plot(df["_dt"], irr, linewidth=2, label=irr_legend)

    # Solo unidades en eje Y derecho
    ax2.set_ylabel(r"(W/m$^{2}$)")

    # Eje X: ticks EXACTOS del log, alternando (una sí, una no) y formato HH:MM
    xt = pd.to_datetime(df["_dt"]).dropna().unique()
    ax1.set_xticks(xt[::2])
    ax1.xaxis.set_major_formatter(mdates.DateFormatter("%H:%M"))
    plt.setp(ax1.get_xticklabels(), rotation=45, ha="right")

    # Que el gráfico empiece y acabe justo en los bordes (sin padding)
    ax1.set_xlim(df["_dt"].iloc[0], df["_dt"].iloc[-1])
    ax1.margins(x=0)

    # --- Resaltar diferencia inicio vs fin (batería) ---
    first_idx = bat.first_valid_index()
    last_idx = bat.last_valid_index()

    if first_idx is not None and last_idx is not None:
        x0 = df.loc[first_idx, "_dt"]
        x1 = df.loc[last_idx, "_dt"]
        y0 = float(bat.loc[first_idx])
        y1 = float(bat.loc[last_idx])

        # Sombreado entre el valor inicial y final
        y_low, y_high = sorted([y0, y1])
        ax1.axhspan(y_low, y_high, color=bat_color, alpha=0.12)

        # Puntos inicio/fin
        ax1.scatter([x0, x1], [y0, y1], color=bat_color, s=35, zorder=5)

        # Línea punteada inicio-fin
        ax1.plot([x0, x1], [y0, y1], color=bat_color, linewidth=1, linestyle=":")

        # Texto ΔV con 3 decimales
        dV = y1 - y0
        ax1.annotate(
            f"ΔV = {dV:+.3f} V",
            xy=(x1, y1),
            xytext=(-10, 10),
            textcoords="offset points",
            ha="right",
            color=bat_color
        )

    # Leyenda (solo nombres, sin unidades)
    ax1.legend([l1, l2], [l1.get_label(), l2.get_label()], loc="best")

    # Sin título
    plt.tight_layout()

    if args.out:
        plt.savefig(args.out, dpi=200, bbox_inches="tight")
        print(f"Guardado: {args.out}")

    if args.show or not args.out:
        plt.show()


if __name__ == "__main__":
    main()
